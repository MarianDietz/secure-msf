/**
 \file 		timer.h
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

#ifndef __MSFTIMER_H__
#define __MSFTIMER_H__

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <ENCRYPTO_utils/timer.h>

class CSocket;

//Note do not change P_FIRST and P_LAST and keep them pointing to the first and last element in the enum
enum MSFPHASE {
	MP_TOTAL, MP_ABY, MP_SETUP, MP_NETWORK, MP_FIRST = MP_TOTAL, MP_LAST = MP_NETWORK
};

extern aby_timings mp_tTimes[MP_LAST - MP_FIRST + 1];
extern aby_comm mp_tSend[MP_LAST - MP_FIRST + 1];
extern aby_comm mp_tRecv[MP_LAST - MP_FIRST + 1];

void MSFResetTimers();

/**
 * Start measuring runtime for a given phase
 * @param msg - a message for debugging
 * @param phase - the ABY phase to measure
 */
void MSFStartWatch(const std::string& msg, MSFPHASE phase);

/**
 * Stop measuring runtime
 * Called after StartWatch() with identical phase parameter
 * @param msg - a message for debugging
 * @param phase - the ABY phase to measure
 */
void MSFStopWatch(const std::string& msg, MSFPHASE phase);

/**
 * Start measuring both runtime and communication
 * @param msg - a message for debugging
 * @param phase - the ABY phase to measure
 * @param sock - a vector of sockets
 */
void MSFStartRecording(const std::string& msg, MSFPHASE phase,
		const std::vector<std::unique_ptr<CSocket>>& sock);

/**
 * Stop measuring both runtime and communication
 * Called after StartRecording() with identical phase parameter
 * @param msg - a message for debugging
 * @param phase - the ABY phase to measure
 * @param sock - a vector of sockets
 */
void MSFStopRecording(const std::string& msg, MSFPHASE phase,
		const std::vector<std::unique_ptr<CSocket>>& sock);

#endif /* MSFTIMER_H_ */
