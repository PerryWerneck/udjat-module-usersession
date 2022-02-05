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

 static const struct {
	const char *name;
	const char *description;
 } events[] = {
	{ "Already active",	"Session is active on startup"		},
	{ "Still active",	"Session still active on shutdown"	},
	{ "Login",			"User has logged in"				},
	{ "Logout",			"User has logged out"				},
	{ "Lock",			"Session was locked"				},
	{ "Unlock",			"Session was unlocked"				},
	{ "Foreground",		"Session is in foreground"			},
	{ "Background",		"Session is in background"			},
	{ "sleep",			"Session is preparing to sleep"		},
	{ "resume",			"Session is resuming from sleep"	},
	{ "shutdown",		"Session is shutting down"			}
 };

 namespace Udjat {

	UDJAT_API User::Event User::EventFactory(const char *name) {
		for(size_t ix = 0; ix < (sizeof(events)/sizeof(events[0])); ix++) {
			if(!strcasecmp(name,events[ix].name)) {
				return (User::Event) ix;
			}
		}
		for(size_t ix = 0; ix < (sizeof(events)/sizeof(events[0])); ix++) {
			if(!strcasecmp(name,events[ix].description)) {
				return (User::Event) ix;
			}
		}
		throw system_error(EINVAL,system_category(),string{"Cant identify event '"} + name + "'");
	}

 }

 namespace std {

 	const char * to_string(const Udjat::User::Event event, bool description) noexcept {

 		if(event > (sizeof(events)/sizeof(events[0]))) {
			return description ? "Invalid event id" : "invalid";
 		}

 		return description ? events[event].description : events[event].name;

 	}

 }
