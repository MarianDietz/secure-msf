#ifndef __MSF_H_
#define __MSF_H_

#include "../../../abycore/circuit/booleancircuits.h"
#include "../../../abycore/circuit/arithmeticcircuits.h"
#include "../../../abycore/circuit/circuit.h"
#include "../../../abycore/aby/abyparty.h"
#include <math.h>
#include <cassert>


int32_t msf(e_role role, const std::string& address, uint16_t port, seclvl seclvl, uint32_t nthreads, std::string& stats_path);

int32_t genOTs(e_role role, const std::string& address, uint16_t port, seclvl seclvl, uint32_t nthreads, std::string& stats_path, uint64_t num);

int32_t test_connectivity(e_role role, const std::string& address, uint16_t port, seclvl seclvl, uint32_t nhtreads, std::string& stats_path, int size, int simd);

int32_t test_subgraph(e_role role, const std::string& address, uint16_t port, seclvl seclvl, uint32_t nthreads, std::string& stats_path, int size, int simd);


#endif
