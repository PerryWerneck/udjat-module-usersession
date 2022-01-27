/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2021 Perry Werneck <perry.werneck@gmail.com>
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

 #include <udjat/tools/usersession.h>
 #include <cstring>
 #include <stdexcept>
 #include <iostream>

 using namespace std;

 namespace Udjat {

	static const char * EventNames[] = {
		"Already active",	// Session is active on controller startup.
		"Still active",		// Session is active on controller shutdown.
		"Login",			// User logon detected.
		"Logout",			// User logoff detected.
		"Lock",				// Session was locked.
		"Unlock",			// Session was unlocked.
		"Foreground",		// Session is in foreground.
		"Background",		// Session is in background.
		"sleep",			// System is preparing to sleep.
		"resume",			// System is resuming from sleep.
		"shutdown",			// System is shutting down.
	};

	static const char * EventDescriptions[] = {
		"Session is active on startup",
		"Session still active on shutdown",
		"User has logged in",
		"User has logged out",
		"Session was locked",
		"Session was unlocked",
		"Session is in foreground",
		"Session is in background",
		"Session is preparing to sleep",
		"Session is resuming from sleep",
		"Session is shutting down",
	};

	UDJAT_API User::Event User::EventFactory(const char *name) {
		for(size_t ix = 0; ix < (sizeof(EventNames)/sizeof(EventNames[0])); ix++) {
			if(!strcasecmp(name,EventNames[ix])) {
				return (User::Event) ix;
			}
		}
		for(size_t ix = 0; ix < (sizeof(EventDescriptions)/sizeof(EventDescriptions[0])); ix++) {
			if(!strcasecmp(name,EventDescriptions[ix])) {
				return (User::Event) ix;
			}
		}
		throw system_error(EINVAL,system_category(),string{"Cant identify event '"} + name + "'");
	}

	UDJAT_API const char * User::EventName(User::Event event) noexcept {
		if(event < (sizeof(EventNames)/sizeof(EventNames[0]))) {
			return EventNames[event];
		}
		return "Undefined";
	}

	UDJAT_API const char * User::EventDescription(User::Event event) noexcept {
		if(event < (sizeof(EventDescriptions)/sizeof(EventDescriptions[0]))) {
			return EventDescriptions[event];
		}
		return "Unknown event";
	}

 }

