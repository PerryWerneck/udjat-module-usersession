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
 #include <iostream>

 using namespace std;

 static const char *statenames[] = {
	"background",
	"foreground",
	"closing",

	"unknown",
 };

 namespace Udjat {

	User::State User::StateFactory(const char *statename) {

#ifdef DEBUG
		cout << "user\tSearching for state '" << statename << "'" << endl;
#endif // DEBUG

		// logind status for 'not in foreground' is 'online'.
		if(!strcasecmp(statename,"online")) {
			return User::SessionInBackground;
		}

		// logind status for 'in foreground' is 'active'
		if(!strcasecmp(statename,"active")) {
			return User::SessionInForeground;
		}

		for(size_t ix = 0; ix < (sizeof(statenames)/sizeof(statenames[0]));ix++) {
			if(!strcasecmp(statename,statenames[ix])) {
				return (User::State) ix;
			}
		}

		cerr << "user\tUnexpected session state '" << statename << "'" << endl;

		return SessionInUnknownState;

	}

	User::Session & User::Session::onEvent(const User::Event &event) noexcept {
#ifdef DEBUG
		cout << "session\t sid=" << this->sid << " Event=" << (int) event
				<< " Alive=" << (alive() ? "Yes" : "No")
				<< " Remote=" << (remote() ? "Yes" : "No")
				<< " User=" << to_string()
				<< endl;
#endif // DEBUG
		return *this;
	}

	User::Session & User::Session::set(User::State state) {

		if(state != this->state.value) {

			cout << to_string() << "\tState changes from '" << this->state.value << "' to '" << state << "'" << endl;
			this->state.value = state;

			if(this->state.value == SessionInForeground) {
				onEvent(foreground);
			} else if(this->state.value == SessionInBackground) {
				onEvent(background);
			}

		}

		return *this;
	}

 }

 namespace std {

	UDJAT_API const char * to_string(const Udjat::User::State state) noexcept {
		if((size_t) state > (sizeof(statenames)/sizeof(statenames[0]))) {
			return "unkown";
		}
		return statenames[state];
	}

 }

