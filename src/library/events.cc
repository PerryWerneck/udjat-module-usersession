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

 #include <config.h>
 #include <udjat/tools/usersession.h>
 #include <udjat/tools/intl.h>
 #include <udjat/tools/logger.h>
 #include <udjat/tools/string.h>
 #include <cstring>
 #include <stdexcept>
 #include <iostream>

 using namespace std;

 static const struct {
 	Udjat::User::Event event;
	const char *name;
	const char *description;
 } events[] = {
	{ Udjat::User::Event::already_active,	N_( "Already active" ),	N_( "Session is active on startup" )		},
	{ Udjat::User::Event::still_active, 	N_( "Still active" ),	N_( "Session still active on shutdown" )	},
	{ Udjat::User::Event::logon, 			N_( "Login" ),			N_( "User has logged in" )					},
	{ Udjat::User::Event::logoff, 			N_( "Logout" ),			N_( "User has logged out" )					},
	{ Udjat::User::Event::lock, 			N_( "Lock" ),			N_( "Session was locked" )					},
	{ Udjat::User::Event::unlock, 			N_( "Unlock" ),			N_( "Session was unlocked" )				},
	{ Udjat::User::Event::foreground, 		N_( "Foreground" ),		N_( "Session is in foreground" )			},
	{ Udjat::User::Event::background,		N_( "Background" ),		N_( "Session is in background" )			},
	{ Udjat::User::Event::sleep,			N_( "sleep" ),			N_( "Session is preparing to sleep" )		},
	{ Udjat::User::Event::resume,			N_( "resume" ),			N_( "Session is resuming from sleep" )		},
	{ Udjat::User::Event::shutdown,			N_( "shutdown" ),		N_( "Session is shutting down" )			},
	{ Udjat::User::Event::pulse,			N_( "pulse" ),			N_( "Pulse" )								},
 };

 namespace Udjat {

	UDJAT_API User::Event User::EventFactory(const char *name) {

		unsigned int rc = 0;
		std::vector<String> names{Udjat::String{name}.split(",")};

		for(auto name : names) {

			name.strip();

			for(size_t ix = 0; ix < (sizeof(events)/sizeof(events[0])); ix++) {
				if(!strcasecmp(name.c_str(),events[ix].name)) {
					rc |= events[ix].event;
				} else if(!strcasecmp(name.c_str(),events[ix].description)) {
					rc |= events[ix].event;
				}
#if defined(GETTEXT_PACKAGE)
				else if(!strcasecmp(name.c_str(),dgettext(GETTEXT_PACKAGE,events[ix].name))) {
					rc |= events[ix].event;
				} else if(!strcasecmp(name.c_str(),dgettext(GETTEXT_PACKAGE,events[ix].description))) {
					rc |= events[ix].event;
				}
#endif // GETTEXT_PACKAGE
			}

		}

		if(rc) {
			return (User::Event) rc;
		}

		throw system_error(EINVAL,system_category(),Logger::Message("Can't parse events '{}'",name));
	}


	Udjat::User::Event User::EventFactory(const pugi::xml_node &node) {

		const char * names = node.attribute("events").as_string();

		if(!*names) {
			names = node.attribute("event").as_string();
		}

		if(!*names) {
			// Last resource, use the alert name.
			names = node.attribute("name").as_string();
		}

		if(!*names) {
			throw runtime_error("Required attribute 'events' is missing");
		}

		return Udjat::User::EventFactory(names);

	}

 }

 namespace std {

 	const std::string to_string(const Udjat::User::Event event, bool description) noexcept {

		std::string rc;

		for(size_t ix = 0; ix < (sizeof(events)/sizeof(events[0])); ix++) {

			if(event & events[ix].event) {

				if(!rc.empty()) {
					rc += ", ";
				}

#if defined(GETTEXT_PACKAGE)
				rc += dgettext(GETTEXT_PACKAGE, description ? events[ix].description : events[ix].name);
#else
				rc += description ? events[ix].description : events[ix].name;
#endif

			}

		}

 		return rc;

 	}

 }
