/**
 \file 		innerproduct_test.cpp
 \author	sreeram.sadasivam@cased.de
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
 \brief		Inner Product Test class implementation.
 */

//Utility libs
#include <ENCRYPTO_utils/crypto/crypto.h>
#include <ENCRYPTO_utils/parse_options.h>
//ABY Party class
#include "../../abycore/aby/abyparty.h"

#include "common/msf.h"

int32_t read_test_options(int32_t* argcp, char*** argvp, e_role* role,
		uint32_t* secparam, std::string* address,
		uint16_t* port, uint32_t* nthreads, std::string* testing, uint32_t* size, uint32_t* count, std::string* stats_path) {

	uint32_t int_role = 0, int_port = 0;

	parsing_ctx options[] =
			{ { (void*) &int_role, T_NUM, "r", "Role: 0/1", true, false },
			  { (void*) secparam, T_NUM, "s", "Symmetric Security Bits, default: 128", false, false },
			  {	(void*) address, T_STR, "a", "IP-address, default: localhost", false, false },
			  {	(void*) &int_port, T_NUM, "p", "Port, default: 7766", false, false },
			  {	(void*) nthreads, T_NUM, "c", "Number of CPUs, default: 2", false, false },
			  {	(void*) testing, T_STR, "t", "Testing (msf/genots/connectivity/subgraph), default: msf", false, false },
			  {	(void*) size, T_NUM, "k", "Size of test, default: 2", false, false },
			  {	(void*) count, T_NUM, "n", "Number of repititions (number of AND's for genots; or simd length for connectivity/subgraph), default: 1", false, false },
			  {	(void*) stats_path, T_STR, "f", "Path for stats output file", false, false },
			};

	if (!parse_options(argcp, argvp, options,
			sizeof(options) / sizeof(parsing_ctx))) {
		print_usage(*argvp[0], options, sizeof(options) / sizeof(parsing_ctx));
		std::cout << "Exiting" << std::endl;
		exit(0);
	}

	assert(int_role < 2);
	*role = (e_role) int_role;

	if (int_port != 0) {
		assert(int_port < 1 << (sizeof(uint16_t) * 8));
		*port = (uint16_t) int_port;
	}

	return 1;
}

int main(int argc, char** argv) {

	e_role role;
	uint32_t bitlen = 16, secparam = 128;
	uint16_t port = 7766;
	std::string address = "127.0.0.1";
	std::string testing = "msf";
	uint32_t size = 2;
	uint32_t count = 1;
	uint32_t nthreads = 2;
	std::string stats_path = "";

	read_test_options(&argc, &argv, &role, &secparam, &address, &port, &nthreads, &testing, &size, &count, &stats_path);

	seclvl seclvl = get_sec_lvl(secparam);

	if (testing == "msf") {
		msf(role, address, port, seclvl, nthreads, stats_path);
	} else if (testing == "genots") {
		genOTs(role, address, port, seclvl, nthreads, stats_path, count);
	} else if (testing == "connectivity") {
		test_connectivity(role, address, port, seclvl, nthreads, stats_path, size, count);
	} else if (testing == "subgraph") {
		test_subgraph(role, address, port, seclvl, nthreads, stats_path, size, count);
	}

	return 0;
}

