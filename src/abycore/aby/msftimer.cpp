/**
 \file 		timer.cpp
 \author 	michael.zohner@ec-spride.de
 \copyright	ABY - A Framework for Efficient Mixed-protocol Secure Two-party Computation
			Copyright (C) 2019 ENCRYPTO Group, TU Darmstadt
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
 \brief		timer Implementation
 */

#include <sys/time.h>
#include <string>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <ENCRYPTO_utils/constants.h>
#include <ENCRYPTO_utils/socket.h>
#include <ENCRYPTO_utils/typedefs.h>

#include "msftimer.h"


aby_timings mp_tTimes[MP_LAST - MP_FIRST + 1];
aby_comm mp_tSend[MP_LAST - MP_FIRST + 1];
aby_comm mp_tRecv[MP_LAST - MP_FIRST + 1];

void MSFResetTimers() {
	mp_tTimes[MP_TOTAL].timing = mp_tTimes[MP_ABY].timing = mp_tTimes[MP_SETUP].timing = mp_tTimes[MP_NETWORK].timing = 0;
	mp_tSend[MP_TOTAL].totalcomm = mp_tSend[MP_ABY].totalcomm = mp_tSend[MP_SETUP].totalcomm = mp_tSend[MP_NETWORK].totalcomm = 0;
	mp_tRecv[MP_TOTAL].totalcomm = mp_tRecv[MP_ABY].totalcomm = mp_tRecv[MP_SETUP].totalcomm = mp_tRecv[MP_NETWORK].totalcomm = 0;
}

void MSFStartWatch(const std::string& msg, MSFPHASE phase) {
	if (phase < MP_FIRST || phase > MP_LAST) {
		std::cerr << "Phase not recognized: " << phase << std::endl;
		return;
	}

	clock_gettime(CLOCK_MONOTONIC, &(mp_tTimes[phase].tbegin));
#ifndef BATCH
	std::cout << msg << std::endl;
#else
	(void)msg;  // silence -Wunused-parameter warning
#endif
}


void MSFStopWatch(const std::string& msg, MSFPHASE phase) {
	if (phase < MP_FIRST || phase > MP_LAST) {
		std::cerr << "Phase not recognized: " << phase << std::endl;
		return;
	}

	clock_gettime(CLOCK_MONOTONIC, &(mp_tTimes[phase].tend));
	mp_tTimes[phase].timing += getMillies(mp_tTimes[phase].tbegin, mp_tTimes[phase].tend);

#ifndef BATCH
	std::cout << msg << mp_tTimes[phase].timing << " ms " << std::endl;
#else
	(void)msg;  // silence -Wunused-parameter warning
#endif
}

void MSFStartRecording(const std::string& msg, MSFPHASE phase,
		const std::vector<std::unique_ptr<CSocket>>& sock) {
	MSFStartWatch(msg, phase);

	mp_tSend[phase].cbegin = 0;
	mp_tRecv[phase].cbegin = 0;
	for(uint32_t i = 0; i < sock.size(); i++) {
		mp_tSend[phase].cbegin += sock[i]->getSndCnt();
		mp_tRecv[phase].cbegin += sock[i]->getRcvCnt();
	}
}

void MSFStopRecording(const std::string& msg, MSFPHASE phase,
		const std::vector<std::unique_ptr<CSocket>>& sock) {
	MSFStopWatch(msg, phase);

	mp_tSend[phase].cend = 0;
	mp_tRecv[phase].cend = 0;
	for(uint32_t i = 0; i < sock.size(); i++) {
		mp_tSend[phase].cend += sock[i]->getSndCnt();
		mp_tRecv[phase].cend += sock[i]->getRcvCnt();
	}

	mp_tSend[phase].totalcomm += mp_tSend[phase].cend - mp_tSend[phase].cbegin;
	mp_tRecv[phase].totalcomm += mp_tRecv[phase].cend - mp_tRecv[phase].cbegin;
}
