#ifndef __MSF_H_
#define __MSF_H_

#include "../../../abycore/circuit/booleancircuits.h"
#include "../../../abycore/circuit/arithmeticcircuits.h"
#include "../../../abycore/circuit/circuit.h"
#include "../../../abycore/aby/abyparty.h"
#include <math.h>
#include <cassert>


int32_t msf(e_role role, const std::string& address, uint16_t port, seclvl seclvl, uint32_t nthreads);

int32_t test_connectivity(e_role role, const std::string& address, uint16_t port, seclvl seclvl, uint32_t nhtreads, int size, int simd);


#endif
