/**
 \file 		innerproduct.cpp
 \author 	sreeram.sadasivam@cased.de
 \copyright	ABY - A Framework for Efficient Mixed-protocol Secure Two-party Computation
 Copyright (C) 2019 Engineering Cryptographic Protocols Group, TU Darmstadt
			This program is free software: you can redistribute it and/or modify
            it under the terms of the GNU Lesser General Public License as published
            by the Free Software Foundation, either version 3 of the License, or
            (at your option) any later version.
            ABY is distributed in the hope that it will be useful,
            but WITHOUT ANY WARRANTY; without even the implied warranty of
            MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
            GNU Lesser General Public License for more details.
            You should have received a copy of the GNU Lesser General Public License
            along with this program. If not, see <http://www.gnu.org/licenses/>.
 \brief		Implementation of the Inner Product using ABY Framework.
 */

#include "msf.h"
#include "../../../abycore/sharing/sharing.h"
#include <set>
#include <chrono>

using namespace std;

typedef uint32_t weight_t;
const size_t weight_bitlen = 32;
const weight_t weight_dummy = -1;

struct Edge {

  int u, v;
  weight_t w;

  Edge(int u, int v, weight_t w) {
    if (u > v) swap(u, v);
    this->u = u;
    this->v = v;
    this->w = w;
  }

  bool operator<(const Edge &o) const {
    if (w != o.w) return w < o.w;
    if (u != o.u) return u < o.u;
    if (v != o.v) return v < o.v;
    return false;
  }

};

int n, m;
vector<int> par;
vector<set<Edge>> adj;

set<int> roots;
vector<vector<int>> component;

set<int> needRecomputing; // vertices whose best incident weight needs to be computed
map<weight_t, set<int>> weightVertices; // maps weight to list of vertices

set<weight_t> needReconnecting; // weights for which we need to run connectivity

int64_t getTime() {
	return chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
}

void recomputeWeights(ABYParty *party) {
	vector<weight_t> w_best_my(needRecomputing.size(), weight_dummy);

	int i = 0;
	for (int u : needRecomputing) {
		if (adj[u].size()) {
			w_best_my[i] = adj[u].begin()->w;
		}
		i++;
	}

	cout << getTime() << ": Start building weight circuit\n";

	std::vector<Sharing*>& sharings = party->GetSharings();
	BooleanCircuit* circ = (BooleanCircuit*) sharings[S_BOOL]->GetCircuitBuildRoutine();

	share *w_best0 = circ->PutSIMDINGate(needRecomputing.size(), w_best_my.data(), weight_bitlen, SERVER);
	share *w_best1 = circ->PutSIMDINGate(needRecomputing.size(), w_best_my.data(), weight_bitlen, CLIENT);
	share *comparison = circ->PutGTGate(w_best1, w_best0);
	share *w_best = circ->PutMUXGate(w_best0, w_best1, comparison);
	share *output = circ->PutOUTGate(w_best, ALL);

	delete w_best0; delete w_best1; delete comparison; delete w_best;

	cout << getTime() << ": Start running weight circuit\n";
	party->ExecCircuit();
	cout << getTime() << ": Finished running weight circuit\n";

	uint32_t *out, bitlen, nvals;
	output->get_clear_value_vec(&out, &bitlen, &nvals);

	i = 0;
	for (int u : needRecomputing) {
		//cout << "found weight" << out[i] << "\n";
		if (out[i] != weight_dummy) {
			weightVertices[out[i]].insert(u);
			needReconnecting.insert(out[i]);
		}
		i++;
	}

	delete output; delete out;
	party->Reset();

	needRecomputing.clear();
}

share* CreateVectorOr(vector<share*> v, BooleanCircuit* circ) {
	while (v.size() > 1) {
		vector<share*> w;
		for (int i = 0; i+1 < v.size(); i += 2) {
			w.push_back(circ->PutORGate(v[i], v[i+1]));
			delete v[i]; delete v[i+1];
		}
		if (v.size() % 2) w.push_back(v.back());
		v = w;
	}
	return v[0];
}

vector<vector<share*>> CreateConnectivityCircuit(weight_t w, BooleanCircuit* circ) {
	set<int> s = weightVertices[w];
	vector<int> vertices(s.begin(), s.end());
	int k = vertices.size();

	cout << "Constructing connectivity of size " << k << "\n";

	map<int, int> inverse;
	for (int i = 0; i < k; ++i)
		inverse[vertices[i]] = i;

	vector<vector<bool>> own(k+1, vector<bool>(k+1, false));
	own[k][k] = true;
	for (int i = 0; i < k; ++i) {
		own[i][i] = true;
		for (const Edge &e : adj[vertices[i]]) {
			if (e.w > w) break;
			int u = par[e.u];
			int v = par[e.v];
			int other = vertices[i] == u ? v : u;
			if (inverse.count(other))
				own[i][inverse[other]] = own[inverse[other]][i] = true;
			else
				own[i][k] = own[k][i] = true;
		}
	}

	vector<vector<share*>> matrix(k+1, vector<share*>(k+1));
	for (int i = 0; i <= k; ++i) {
		for (int j = 0; j <= k; ++j) {
			share *x = circ->PutINGate((uint8_t) own[i][j], 1, SERVER);
			share *y = circ->PutINGate((uint8_t) own[i][j], 1, CLIENT);
			matrix[i][j] = circ->PutORGate(x, y);
			delete x; delete y;
		}
	}

	for (int l = 0; (1<<l) < k; ++l) {
		vector<vector<share*>> nmatrix(k+1, vector<share*>(k+1));
		for (int i = 0; i <= k; ++i) {
			for (int j = 0; j <= k; ++j) {
				vector<share*> v;
				for (int m = 0; m <= k; ++m) {
					v.push_back(circ->PutANDGate(matrix[i][m], matrix[m][j]));
				}
				nmatrix[i][j] = CreateVectorOr(v, circ);
			}
		}
		for (int i = 0; i <= k; ++i) {
			for (int j = 0; j <= k; ++j) {
				delete matrix[i][j];
			}
		}
		matrix = nmatrix;
	}

	for (int i = 0; i <= k; ++i) {
		for (int j = 0; j <= k; ++j) {
			share *out = circ->PutOUTGate(matrix[i][j], ALL);
			delete matrix[i][j];
			matrix[i][j] = out;
		}
	}

	return matrix;
}

vector<vector<int>> FinishConnectivity(weight_t w, vector<vector<share*>> &matrix) {
	set<int> s = weightVertices[w];
	vector<int> vertices(s.begin(), s.end());
	int k = vertices.size();

	vector<vector<int>> res;

	set<int> done;
	for (int i = 0; i < k; ++i) {
		if (done.count(i) || matrix[i][k]->get_clear_value<uint32_t>())
			continue;
		vector<int> cur;
		for (int j = i; j < k; ++j) {
			if (matrix[i][j]->get_clear_value<uint32_t>()) {
				cur.push_back(vertices[j]);
				done.insert(j);
			}
		}
		res.push_back(cur);
	}

	for (int i = 0; i <= k; ++i) {
		for (int j = 0; j <= k; ++j) {
			delete matrix[i][j];
		}
	}

	return res;
}

int merge(vector<int> &comp, weight_t w) {
	//cout << "MERGING:";
	//for (int u : comp) {
	//	cout << " " << u;
	//}
	//cout << "\n";

	for (int u : comp) weightVertices[w].erase(u);
	if (weightVertices[w].empty()) weightVertices.erase(w);

	int r = comp[0];
	for (int i = 1; i < comp.size(); ++i)
		if (component[i].size() > component[r].size())
			r = i;
	
	for (int u : comp) if (u != r) {
		for (int v : component[u]) {
			par[v] = r;
			component[r].push_back(v);
		}
		for (const Edge &e : adj[u])
			adj[r].insert(e);
	}
	
	while (!adj[r].empty() && par[adj[r].begin()->u] == par[adj[r].begin()->v])
		adj[r].erase(adj[r].begin());

	needRecomputing.insert(r);
	return r;
}

void reconnect(ABYParty *party) {
	std::vector<Sharing*>& sharings = party->GetSharings();
	BooleanCircuit* circ = (BooleanCircuit*) sharings[S_BOOL]->GetCircuitBuildRoutine();

	// do not need to do this for size-1 components:
	for (auto it = needReconnecting.begin(); it != needReconnecting.end();) {
		if (weightVertices[*it].size() <= 1)
			it = needReconnecting.erase(it); 
		else ++it;
	}

	cout << getTime() << ": Start building connectivity circuit\n";

	map<weight_t, vector<vector<share*>>> matrices;
	for (weight_t w : needReconnecting)
		matrices[w] = CreateConnectivityCircuit(w, circ);
	
	cout << getTime() << ": Start running connectivity circuit\n";
	party->ExecCircuit();
	cout << getTime() << ": Finished running connectivity circuit\n";

	int firstMerged = -1;
	weight_t secondMerged = weight_dummy;
	for (weight_t w : needReconnecting) {
		vector<vector<int>> components = FinishConnectivity(w, matrices[w]);
		for (vector<int> &comp : components) {
			int r = merge(comp, w);
			if (firstMerged == -1) firstMerged = r;
			else if (secondMerged == weight_dummy) secondMerged = w;
		}
	}
	needReconnecting.clear();
	party->Reset();

	while (!weightVertices.empty() && weightVertices.begin()->first <= secondMerged) {
		int w = weightVertices.begin()->first;
		set<int> s = weightVertices.begin()->second;
		s.insert(firstMerged);
		vector<int> comp(s.begin(), s.end());
		firstMerged = merge(comp, w);
	}
}

int32_t msf(e_role role, const std::string& address, uint16_t port, seclvl seclvl) {
	cin >> n >> m;
	par.resize(n);
	adj.resize(n);
	component.resize(n);

	for (int i = 0; i < n; ++i) {
		par[i] = i;
		roots.insert(i);
		component[i] = {i};
		needRecomputing.insert(i);
	}

	for (int i = 0; i < m; ++i) {
		int u, v, w;
		cin >> u >> v >> w;
		Edge e(u, v, w);
		adj[u].insert(e);
		adj[v].insert(e);
	}

	ABYParty* party = new ABYParty(role, address, port, seclvl);
	party->ConnectAndBaseOTs();

	while (!needRecomputing.empty()) {
		recomputeWeights(party);
		if (!needReconnecting.empty()) reconnect(party);
	}

	delete party;

	return 0;
}
