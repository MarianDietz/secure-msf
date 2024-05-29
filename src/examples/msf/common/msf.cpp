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
#include <random>

using namespace std;

typedef uint32_t weight_t;
const size_t weight_bitlen = 32;
const weight_t weight_dummy = -1;

typedef uint32_t edgecount_t;
const size_t edgecount_bitlen = 32;
const edgecount_t edgecount_dummy = -1;

const size_t kappa = 40;

random_device dev;
mt19937 rng(dev());
uniform_int_distribution<edgecount_t> dist(0, edgecount_dummy);

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

struct SubgraphEdge {

	int u, v;
	int orig_u, orig_v;
	weight_t w;

	SubgraphEdge(const Edge &e) {
		this->u = this->orig_u = e.u;
		this->v = this->orig_v = e.v;
		this->w = e.w;
	}

	SubgraphEdge(int u, int v, int orig_u, int orig_v, weight_t w) {
		this->u = u;
		this->v = v;
		this->orig_u = orig_u;
		this->orig_v = orig_v;
		this->w = w;
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

map<int, vector<vector<vector<vector<SubgraphEdge>>>>> subgraphs;
vector<pair<int, int>> subgraphStats;

uint64_t count_ands_bestweight = 0, count_ands_connectivity = 0, count_ands_subgraph;

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

	//cout << getTime() << ": Start building weight circuit\n";

	std::vector<Sharing*>& sharings = party->GetSharings();
	BooleanCircuit* circ = (BooleanCircuit*) sharings[S_BOOL]->GetCircuitBuildRoutine();

	share *w_best0 = circ->PutSIMDINGate(needRecomputing.size(), w_best_my.data(), weight_bitlen, SERVER);
	share *w_best1 = circ->PutSIMDINGate(needRecomputing.size(), w_best_my.data(), weight_bitlen, CLIENT);
	share *comparison = circ->PutGTGate(w_best1, w_best0);
	share *w_best = circ->PutMUXGate(w_best0, w_best1, comparison);
	share *output = circ->PutOUTGate(w_best, ALL);

	delete w_best0; delete w_best1; delete comparison; delete w_best;

	//cout << getTime() << ": Start running weight circuit\n";
	count_ands_bestweight += circ->GetNumANDGates();
	party->ExecCircuit();
	//cout << getTime() << ": Finished running weight circuit\n";

	uint32_t *out, bitlen, nvals;
	output->get_clear_value_vec(&out, &bitlen, &nvals);

	i = 0;
	for (int u : needRecomputing) {
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

void populateConnectivityData(weight_t w, vector<vector<vector<uint8_t>>> &data) {
	set<int> s = weightVertices[w];
	vector<int> vertices(s.begin(), s.end());
	int k = vertices.size();

	map<int, int> inverse;
	for (int i = 0; i < k; ++i)
		inverse[vertices[i]] = i;

	vector<vector<bool>> own(k+1);
	for (int i = 0; i < k; ++i) data[k][i].push_back(false);
	data[k][k].push_back(true);
	for (int i = 0; i < k; ++i) {
		for (int j = 0; j < i; ++j) data[i][j].push_back(false);
		data[i][i].push_back(true);
		for (const Edge &e : adj[vertices[i]]) {
			if (e.w > w) break;
			int u = par[e.u];
			int v = par[e.v];
			int other = vertices[i] == u ? v : u;
			if (inverse.count(other)) {
				if (inverse[other] < i)
					data[i][inverse[other]].back() = true;
			} else
				data[k][i].back() = true;
		}
	}
}

vector<vector<share*>> CreateConnectivityCircuit(int k, vector<vector<vector<uint8_t>>> &data, BooleanCircuit* circ) {
	vector<vector<share*>> matrix(k+1);
	for (int i = 0; i <= k; ++i) {
		vector<share*> row;
		for (int j = 0; j <= i; ++j) {
			share *x = circ->PutSIMDINGate(data[i][j].size(), data[i][j].data(), 1, SERVER);
			share *y = circ->PutSIMDINGate(data[i][j].size(), data[i][j].data(), 1, CLIENT);
			row.push_back(circ->PutORGate(x, y));
			delete x; delete y;
		}

		for (int j = 0; j < i; ++j) {
			vector<share*> v;
			for (int m = 0; m < i; ++m) {
				v.push_back(circ->PutANDGate(matrix[max(j,m)][min(j,m)], row[m]));
			}
			matrix[i].push_back(CreateVectorOr(v, circ));
		}
		matrix[i].push_back(circ->PutSIMDCONSGate(data[i][i].size(), (UGATE_T)1, 1));

		for (share *r : row) delete r;

		for (int a = 0; a < i; ++a) {
			for (int b = 0; b < a; ++b) {
				share *andd = circ->PutANDGate(matrix[i][a], matrix[i][b]);
				share *res = circ->PutORGate(matrix[a][b], andd);
				delete andd; delete matrix[a][b];
				matrix[a][b] = res;
			}
		}
	}

	for (int i = 0; i <= k; ++i) {
		for (int j = 0; j <= i; ++j) {
			share *out = circ->PutOUTGate(matrix[i][j], ALL);
			delete matrix[i][j];
			matrix[i][j] = out;
		}
	}

	return matrix;
}

vector<vector<int>> FinishConnectivity(weight_t w, int pos, vector<vector<uint32_t*>> &matrix) {
	set<int> s = weightVertices[w];
	vector<int> vertices(s.begin(), s.end());
	int k = vertices.size();

	vector<vector<int>> res;

	// for (int i = 0; i <= k; ++i) {
	// 	for (int j = 0; j <= i; ++j) {
	// 		cout << matrix[i][j][pos] << " ";
	// 	}
	// 	cout << "\n";
	// }

	set<int> done;
	for (int i = 0; i < k; ++i) {
		if (done.count(i) || matrix[k][i][pos])
			continue;
		vector<int> cur;
		cur.push_back(vertices[i]);
		done.insert(i);
		for (int j = i+1; j < k; ++j) {
			if (matrix[j][i][pos]) {
				cur.push_back(vertices[j]);
				done.insert(j);
			}
		}
		res.push_back(cur);
	}

	return res;
}

int merge(vector<int> &comp, weight_t w) {
	// cout << "MERGING:";
	// for (int u : comp) {
	// 	cout << " " << u;
	// }
	// cout << "\n";

	for (int u : comp) weightVertices[w].erase(u);
	if (weightVertices[w].empty()) weightVertices.erase(w);

	int r = comp[0];
	for (int i = 1; i < comp.size(); ++i)
		if (component[comp[i]].size() > component[r].size())
			r = comp[i];
	
	map<int, int> newIndices;
	vector<vector<vector<SubgraphEdge>>> subgraph;
	for (int i = 0; i < comp.size(); ++i) subgraph.emplace_back(i);
	for (int u : comp) {
		int i = newIndices.size();
		newIndices[u] = i;
	}
	for (int u : comp) {
		for (const Edge &e : adj[u]) {
			if (e.w > w) break;
			if (u != par[e.u] || par[e.u] == par[e.v]) continue; // make sure to add each edge only once
			int i = newIndices[par[e.u]], j = newIndices[par[e.v]];
			SubgraphEdge se(e);
			se.u = max(i, j);
			se.v = min(i, j);
			subgraph[se.u][se.v].push_back(se);
		}
	}
	subgraphs[comp.size()].push_back(subgraph);
	
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

	map<int, vector<vector<vector<uint8_t>>>> mp;
	for (weight_t w : needReconnecting) {
		int k = weightVertices[w].size();
		if (mp.count(k) == 0) {
			for (int i = 0; i <= k; ++i) mp[k].emplace_back(i+1);
		}
		populateConnectivityData(w, mp[k]);
	}

	//cout << getTime() << ": Start building connectivity circuit\n";

	map<int, vector<vector<share*>>> matrices;
	for (auto &a : mp) {
		// cout << "building size " << a.first << "\n";
		matrices[a.first] = CreateConnectivityCircuit(a.first, a.second, circ);
	}
	
	//cout << getTime() << ": Start running connectivity circuit\n";
	count_ands_connectivity += circ->GetNumANDGates();
	party->ExecCircuit();
	//cout << getTime() << ": Finished running connectivity circuit\n";

	map<int, vector<vector<uint32_t*>>> mpres;
	for (const auto &a : matrices) {
		int k = a.first;
		for (int i = 0; i <= k; ++i) mpres[k].emplace_back(i+1);
		for (int i = 0; i <= k; ++i) {
			for (int j = 0; j <= i; ++j) {
				uint32_t *out, bitlen, nvals;
				a.second[i][j]->get_clear_value_vec(&mpres[k][i][j], &bitlen, &nvals);
				delete a.second[i][j];
			}
		}
	}

	map<int, int> curpos;

	int firstMerged = -1;
	weight_t secondMerged = weight_dummy;
	for (weight_t w : needReconnecting) {
		int k = weightVertices[w].size();
		int pos = curpos[k]++;
		vector<vector<int>> components = FinishConnectivity(w, pos, mpres[k]);
		for (vector<int> &comp : components) {
			int r = merge(comp, w);
			if (firstMerged == -1) firstMerged = r;
			else if (secondMerged == weight_dummy) secondMerged = w;
		}
	}

	for (auto &a : mpres)
		for (auto &b : a.second)
			for (auto &c : b)
				delete c;
	needReconnecting.clear();
	party->Reset();

	while (firstMerged != -1 && !weightVertices.empty() && weightVertices.begin()->first <= secondMerged) {
		int w = weightVertices.begin()->first;
		set<int> s = weightVertices.begin()->second;
		s.insert(firstMerged);
		vector<int> comp(s.begin(), s.end());
		needRecomputing.erase(firstMerged);
		firstMerged = merge(comp, w);
	}
}

share* CreateRandIntCircuit(share *bound, int simd, BooleanCircuit* circ) {
	vector<uint32_t> &bound_wires = bound->get_wires();
	vector<uint32_t> padded_wires(bound_wires.size());
	padded_wires.back() = bound_wires.back();
	for (int i = padded_wires.size()-2; i >= 0; --i)
		padded_wires[i] = circ->PutORGate(bound_wires[i], padded_wires[i+1]);
	share *padded = new boolshare(padded_wires, circ);

	vector<share*> c;
	vector<share*> ok;
	for (int i = 0; i < kappa; ++i) {
		vector<edgecount_t> data;
		for (int z = 0; z < simd; ++z) data.push_back(dist(rng));
		share *r = circ->PutSharedSIMDINGate(simd, data.data(), edgecount_bitlen);
		share *s = circ->PutANDGate(r, padded);
		c.push_back(s);
		ok.push_back(circ->PutGTGate(bound, s));
		delete r;
	}
	delete padded;

	while (c.size() > 1) {
		vector<share*> cnew;
		vector<share*> oknew;
		for (int i = 0; i+1 < c.size(); i += 2) {
			cnew.push_back(circ->PutMUXGate(c[i], c[i+1], ok[i]));
			oknew.push_back(circ->PutORGate(ok[i], ok[i+1]));
			delete c[i]; delete c[i+1];
			delete ok[i]; delete ok[i+1];
		}
		if (c.size()%2) {
			cnew.push_back(c.back());
			oknew.push_back(ok.back());
		}
		c = cnew;
		ok = oknew;
	}
	delete ok[0];

	return c[0];
}

void CreatePrefixSumCircuit(vector<share*> &a, BooleanCircuit* circ) {
	for (int i = 1; i < a.size(); ++i) {
		a[i] = circ->PutADDGate(a[i-1], a[i]);
		//cout << "depth: " << circ->GetMaxDepth() << "\n";
	}
	// if (a.size() == 1) return;
	// vector<share*> b;
	// for (int i = 1; i < a.size(); i += 2) {
	// 	b.push_back(circ->PutADDGate(a[i-1], a[i]));
	// }
	// CreatePrefixSumCircuit(b, circ);
	// for (int i = 1; i < a.size(); i += 2) {
	// 	a[i] = b[i/2];
	// }
	// for (int i = 2; i < a.size(); i += 2) {
	// 	a[i] = circ->PutADDGate(b[i/2-1], a[i]);
	// }
}

vector<share*> CreateSubgraphCircuit(int k, vector<vector<vector<vector<SubgraphEdge>>>> &e, BooleanCircuit* circ) {
	int vertex_bitlen = 1;
	while ((1 << vertex_bitlen) < k) vertex_bitlen++;

	vector<share*> u;
	for (int i = 0; i < k; ++i)
		u.push_back(circ->PutSIMDCONSGate(e.size(), (UGATE_T)i, vertex_bitlen));

	vector<share*> a;
	vector<share*> s;
	for (int i = 0; i < k; ++i) {
		for (int j = 0; j < i; ++j) {
			vector<edgecount_t> data;
			for (int z = 0; z < e.size(); ++z) data.push_back(e[z][i][j].size());
			a.push_back(circ->PutSIMDINGate(e.size(), data.data(), edgecount_bitlen, SERVER));
			a.push_back(circ->PutSIMDINGate(e.size(), data.data(), edgecount_bitlen, CLIENT));
			s.push_back(circ->PutSIMDCONSGate(e.size(), (UGATE_T)edgecount_dummy, edgecount_bitlen));
			s.push_back(circ->PutSIMDCONSGate(e.size(), (UGATE_T)edgecount_dummy, edgecount_bitlen));
		}
	}

	// if (k == 3) for (int i = 0; i < a.size(); ++i) circ->PutPrintValueGate(a[i], "a[" + to_string(i) + "]");

	share *zero = circ->PutSIMDCONSGate(e.size(), (UGATE_T) 0, edgecount_bitlen);
	for (int z = 0; z < k-1; ++z) {
		// cout << "iteration " << z << "\n";
		vector<share*> apre = a;
		CreatePrefixSumCircuit(apre, circ);

		share *r = CreateRandIntCircuit(apre.back(), e.size(), circ);

		// cout << "step 1\n";

		vector<share*> g(a.size());
		for (int i = 0; i < a.size(); ++i)
			g[i] = circ->PutGTGate(apre[i], r);
		vector<share*> x, y;
		for (int i = 0; i < k; ++i) {
			for (int j = 0; j < i; ++j) {
				x.push_back(u[i]);
				x.push_back(u[i]);
				y.push_back(u[j]);
				y.push_back(u[j]);
			}
		}

		vector<share*> gcur = g;
		while (gcur.size() > 1) {
			vector<share*> gnew;
			vector<share*> xnew, ynew;
			for (int i = 0; i+1 < gcur.size(); i += 2) {
				xnew.push_back(circ->PutMUXGate(x[i], x[i+1], gcur[i]));
				ynew.push_back(circ->PutMUXGate(y[i], y[i+1], gcur[i]));
				gnew.push_back(circ->PutORGate(gcur[i], gcur[i+1]));
				if (gcur.size() != g.size()) {
					delete gcur[i]; delete gcur[i+1];
					delete x[i]; delete x[i+1];
					delete y[i]; delete y[i+1];
				}
			}
			if (gcur.size()%2) {
				gnew.push_back(gcur.back());
				xnew.push_back(x.back());
				ynew.push_back(y.back());
			}
			gcur = gnew;
			x = xnew;
			y = ynew;
		}
		// if (k == 3) for (int i = 0; i < a.size(); ++i) circ->PutPrintValueGate(g[i], "g[" + to_string(i) + "]");

		// cout << "step 2\n";

		for (int i = 0; i < k; ++i) {
			share *eq = circ->PutEQGate(u[i], y[0]);
			share *unew = circ->PutMUXGate(x[0], u[i], eq);
			delete u[i]; delete eq;
			u[i] = unew;
		}

		// if (k == 3) circ->PutPrintValueGate(x[0], "x");
		// if (k == 3) circ->PutPrintValueGate(y[0], "y");
		// if (k == 3) for (int i = 0; i < k; ++i) circ->PutPrintValueGate(u[i], "u[" + to_string(i) + "]");

		delete gcur[0]; delete x[0]; delete y[0];

		// cout << "step 3\n";

		int i = 1, j = 0, p = 0;
		for (int q = 0; q < a.size(); ++q) {
			// if (k == 3) cout << i << " " << j << " " << p << "\n";
			share *eq = circ->PutEQGate(u[i], u[j]);
			share *anew = circ->PutMUXGate(zero, a[q], eq);
			delete eq;
			if (q) delete a[q];
			a[q] = anew;

			share *cond;
			if (q) {
				share *notprev = circ->PutINVGate(g[q-1]);
				cond = circ->PutANDGate(g[q], notprev);
				delete notprev;
			} else cond = g[q];

			share *sub = circ->PutSUBGate(r, q ? apre[q-1] : zero);
			share *snew = circ->PutMUXGate(sub, s[q], cond);
			delete s[q]; delete sub;
			if (q) delete cond;
			s[q] = snew;

			if (p == 0) p++;
			else if (j < i-1) {
				j++;
				p = 0;
			} else {
				i++;
				j = p = 0;
			}
		}

		// cout << "step 4\n";
		// if (k == 3) circ->PutPrintValueGate(r, "r");
		// if (k == 3) for (int i = 0; i < apre.size(); ++i) circ->PutPrintValueGate(apre[i], "apre[" + to_string(i) + "]");

		for (int i = 0; i < a.size(); ++i) delete apre[i];
		for (share *sh : g) delete sh;
		delete r;

		// cout << "step 5\n";

		// if (k == 3) for (int i = 0; i < s.size(); ++i) circ->PutPrintValueGate(s[i], "s[" + to_string(i) + "]");
	}
	delete zero;

	for (int i = 0; i < k; ++i) delete u[i];
	
	vector<share*> outs;
	for (int i = 0; i < s.size(); ++i) {
		outs.push_back(circ->PutOUTGate(s[i], i%2 ? CLIENT : SERVER));
		delete s[i]; delete a[i];
	}

	return outs;
}

void findSubgraphMSFs(ABYParty *party, e_role role) {
	std::vector<Sharing*>& sharings = party->GetSharings();
	BooleanCircuit* circ = (BooleanCircuit*) sharings[S_BOOL]->GetCircuitBuildRoutine();

	//cout << getTime() << ": Start building subgraph circuit\n";

	map<int, vector<share*>> outputs;
	for (auto &a : subgraphs) {
		//cout << "building " << a.second.size() << " subgraphs of size " << a.first << "\n";
		subgraphStats.push_back({a.first, a.second.size()});
		outputs[a.first] = CreateSubgraphCircuit(a.first, a.second, circ);
		//for (auto &e : a) cout << e.orig_u << "-" << e.orig_v << " ";
		//cout << "\n";
	}

	//cout << getTime() << ": Start running subgraph circuit\n";
	count_ands_subgraph += circ->GetNumANDGates();
	party->ExecCircuit();
	//cout << getTime() << ": Finished running subgraph circuit\n";

	for (auto &a : subgraphs) {
		int k = a.first;
		int pos = 0;
		vector<vector<uint64_t*>> clear;
		for (int i = 0; i < k; ++i) {
			clear.emplace_back(i);
			for (int j = 0; j < i; ++j) {
				for (int p = 0; p < 2; ++p) {
					if ((role == SERVER && p == 0) || (role == CLIENT && p == 1)) {
						uint64_t *out;
						uint32_t bitlen, nvals;
						outputs[k][pos]->get_clear_value_vec(&out, &bitlen, &nvals);
						clear[i][j] = out;
					}
					pos++;
				}
			}
		}
		for (int z = 0; z < a.second.size(); ++z) {
			vector<vector<vector<SubgraphEdge>>> &subgraph = a.second[z];
			for (int i = 0; i < k; ++i) {
				for (int j = 0; j < i; ++j) {
					edgecount_t val = clear[i][j][z];
					if (val != edgecount_dummy) {
						SubgraphEdge &e = subgraph[i][j][val];
						//cout << "found edge " << e.orig_u << "-" << e.orig_v << " of weight " << e.w << "\n";
					}
				}
			}
		}
	}

	party->Reset();
}

int32_t msf(e_role role, const std::string& address, uint16_t port, seclvl seclvl, uint32_t nthreads, std::string& stats_path) {
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

	ABYParty* party = new ABYParty(role, address, port, seclvl, 32, nthreads);
	party->ConnectAndBaseOTs();
	party->GetSharings()[S_BOOL]->SetPreCompPhaseValue(ePreCompRead);

	MSFResetTimers();

	MSFStartWatch("Total", MP_TOTAL);

	cout << "round ";
	int rounds = 1;
	while (!needRecomputing.empty()) {
		cout << rounds << " " << flush;
		recomputeWeights(party);
		if (!needReconnecting.empty()) reconnect(party);
		rounds++;
	}
	cout << "\n";

	ofstream stats;
	stats.open(stats_path);

	MSFStopWatch("Total", MP_TOTAL);
	stats << "iterations " << rounds-1 << "\n";
	stats << "phase1(total/aby/setup/network) " << mp_tTimes[MP_TOTAL].timing << " " << mp_tTimes[MP_ABY].timing << " " << mp_tTimes[MP_SETUP].timing << " " << mp_tTimes[MP_NETWORK].timing << "\n";
	stats << "phase1(recv/send)" << " " << mp_tRecv[MP_ABY].totalcomm << " " << mp_tSend[MP_ABY].totalcomm << "\n";
	// cout << "Total time: " << mp_tTimes[MP_TOTAL].timing << " ms" << "\n";
	// cout << "ABY total time: " << mp_tTimes[MP_ABY].timing << " ms" << "\n";
	// cout << "Setup time: " << mp_tTimes[MP_SETUP].timing << " ms" << "\n";
	// cout << "Network time: " << mp_tTimes[MP_NETWORK].timing << " ms" << "\n";
	MSFResetTimers();

	cout << "subgraphs...\n";
	findSubgraphMSFs(party, role);

	MSFStopWatch("Total", MP_TOTAL);
	stats << "phase2(total/aby/setup/network) " << mp_tTimes[MP_TOTAL].timing << " " << mp_tTimes[MP_ABY].timing << " " << mp_tTimes[MP_SETUP].timing << " " << mp_tTimes[MP_NETWORK].timing << "\n";
	stats << "phase2(recv/send)" << " " << mp_tRecv[MP_ABY].totalcomm << " " << mp_tSend[MP_ABY].totalcomm << "\n";
	// cout << "Total time: " << mp_tTimes[MP_TOTAL].timing << " ms" << "\n";
	// cout << "ABY total time: " << mp_tTimes[MP_ABY].timing << " ms" << "\n";
	// cout << "Setup time: " << mp_tTimes[MP_SETUP].timing << " ms" << "\n";
	// cout << "Network time: " << mp_tTimes[MP_NETWORK].timing << " ms" << "\n";

	stats << "ands(computew,connectivity,subgraph) " << count_ands_bestweight << " " << count_ands_connectivity << " " << count_ands_subgraph << "\n";

	stats << "subgraphs";
	for (auto a : subgraphStats) stats << " " << a.first << ":" << a.second;
	stats << "\n";

	stats.close();

	delete party;

	return 0;
}

int32_t genOTs(e_role role, const std::string& address, uint16_t port, seclvl seclvl, uint32_t nthreads, std::string& stats_path, uint64_t num) {
	ABYParty* party = new ABYParty(role, address, port, seclvl, 32, nthreads);
	party->ConnectAndBaseOTs();

	MSFResetTimers();
	MSFStartWatch("Total", MP_TOTAL);

	party->ExecSetup(num);

	MSFStopWatch("Total", MP_TOTAL);

	ofstream stats;
	stats.open(stats_path);

	stats << "ots(total) " << mp_tTimes[MP_TOTAL].timing << "\n";
	stats << "ots(recv/send)" << " " << mp_tRecv[MP_ABY].totalcomm << " " << mp_tSend[MP_ABY].totalcomm << "\n";

	stats.close();

	delete party;

	return 0;
}

int32_t test_connectivity(e_role role, const std::string& address, uint16_t port, seclvl seclvl, uint32_t nthreads, std::string& stats_path, int size, int simd) {
	ABYParty* party = new ABYParty(role, address, port, seclvl, 32, nthreads);
	party->ConnectAndBaseOTs();
	party->GetSharings()[S_BOOL]->SetPreCompPhaseValue(ePreCompRead);

	MSFResetTimers();
	MSFStartWatch("Total", MP_TOTAL);

	std::vector<Sharing*>& sharings = party->GetSharings();
	BooleanCircuit* circ = (BooleanCircuit*) sharings[S_BOOL]->GetCircuitBuildRoutine();

	//cout << getTime() << ": Start building connectivity circuit\n";

	vector<vector<vector<uint8_t>>> data;
	for (int i = 0; i <= size; ++i) {
		data.emplace_back(i+1);
		for (int j = 0; j <= i; ++j) data[i][j].resize(simd, 1);
	}
	CreateConnectivityCircuit(size, data, circ);
	
	//cout << getTime() << ": Start running connectivity circuit\n";
	party->ExecCircuit();
	//cout << getTime() << ": Finished running connectivity circuit\n";

	MSFStopWatch("Total", MP_TOTAL);

	ofstream stats;
	stats.open(stats_path);

	stats << "connectivity(total) " << mp_tTimes[MP_TOTAL].timing << "\n";
	stats << "ands " << count_ands_connectivity << "\n";
	stats << "connectivity(recv/send)" << " " << mp_tRecv[MP_ABY].totalcomm << " " << mp_tSend[MP_ABY].totalcomm << "\n";

	stats.close();

	delete party;

	return 0;
}

int32_t test_subgraph(e_role role, const std::string& address, uint16_t port, seclvl seclvl, uint32_t nthreads, std::string& stats_path, int size, int simd) {
	ABYParty* party = new ABYParty(role, address, port, seclvl, 32, nthreads);
	party->ConnectAndBaseOTs();
	party->GetSharings()[S_BOOL]->SetPreCompPhaseValue(ePreCompRead);

	MSFResetTimers();
	MSFStartWatch("Total", MP_TOTAL);

	std::vector<Sharing*>& sharings = party->GetSharings();
	BooleanCircuit* circ = (BooleanCircuit*) sharings[S_BOOL]->GetCircuitBuildRoutine();

	//cout << getTime() << ": Start building subgraph circuit\n";

	vector<vector<vector<vector<SubgraphEdge>>>> subgraphs;
	for (int i = 0; i < simd; ++i) {
		subgraphs.emplace_back(size);
		for (int j = 0; j < size; ++j) {
			subgraphs.back()[j].resize(j);
			for (int k = 0; k < j; ++k)
				subgraphs.back()[j][k].push_back(SubgraphEdge(j, k, j, k, 0));
		}
	}
	CreateSubgraphCircuit(size, subgraphs, circ);

	//cout << getTime() << ": Start running subgraph circuit\n";
	party->ExecCircuit();
	//cout << getTime() << ": Finished running subgraph circuit\n";

	MSFStopWatch("Total", MP_TOTAL);

	ofstream stats;
	stats.open(stats_path);

	stats << "subgraph(total) " << mp_tTimes[MP_TOTAL].timing << "\n";
	stats << "ands " << count_ands_subgraph << "\n";
	stats << "subgraph(recv/send)" << " " << mp_tRecv[MP_ABY].totalcomm << " " << mp_tSend[MP_ABY].totalcomm << "\n";

	stats.close();

	delete party;

	return 0;
}