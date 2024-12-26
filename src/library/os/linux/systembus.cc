/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2024 Perry Werneck <perry.werneck@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

 /**
  * @brief Implementing singleton for sd_bus_system.
  */

/*
 #include <config.h>
 #include <udjat/defs.h>
 #include <stdexcept>

 #include "private.h"

 using namespace std;

 #ifdef HAVE_DBUS
 namespace Udjat {

	SystemBus::SystemBus() {
		int rc = sd_bus_default_system(&connct);

		if(rc < 0) {
			throw system_error(-rc,system_category(),string{"Unable to open system bus (rc="}+std::to_string(rc)+")");
		}

		if(Logger::enabled(Logger::Debug)) {
			Logger::String{"Got system bus on socket ",sd_bus_get_fd(connct)}.write(Logger::Debug,"sdsysbus");
		}
	}

	SystemBus::~SystemBus() {
		sd_bus_flush(connct);
		if(Logger::enabled(Logger::Debug)) {
			Logger::String{"Released system bus from socket ",sd_bus_get_fd(connct)}.write(Logger::Debug,"sdsysbus");
		}
		sd_bus_unrefp(&connct);
	}

	SystemBus & SystemBus::getInstance() {
		static SystemBus instance;
		return instance;
	}

 }
 #endif // HAVE_DBUS
*/
