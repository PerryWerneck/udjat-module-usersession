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
 #include <udjat/tools/intl.h>
 #include <udjat/tools/usersession.h>
 #include <iostream>
 #include <cstring>

 using namespace std;

 static const char *statenames[] = {
	N_( "background" ),
	N_( "foreground" ),
	N_( "closing" ),

	N_( "unknown" ),
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

		// logind status is 'openint'
		if(!strcasecmp(statename,"opening")) {
			return User::SessionIsOpening;
		}

		cerr << "user\tUnexpected session state '" << statename << "'" << endl;

		return SessionInUnknownState;

	}

	void User::Session::emit(const Event &event) noexcept {
		onEvent(event);
	}


	User::Session & User::Session::onEvent(const User::Event &event) noexcept {
#ifdef DEBUG
		cout << "session\t**EVENT** sid=" << this->sid << " Event=" << (int) event
				<< " Alive=" << (alive() ? "Yes" : "No")
				<< " Remote=" << (remote() ? "Yes" : "No")
				<< " User=" << to_string()
				<< endl;
#endif // DEBUG
		return *this;
	}

	User::Session & User::Session::set(User::State state) {

		if(state != this->flags.state) {

			cout << to_string() << "\tState changes from '" << this->flags.state << "' to '" << state << "'" << endl;
			this->flags.state = state;

			try {

				if(this->flags.state == SessionInForeground) {
					emit(User::foreground);
				} else if(this->flags.state == SessionInBackground) {
					emit(User::background);
				}

			} catch(const std::exception &e) {

				cerr << to_string() << "\tError '" << e.what() << "' updating state" << endl;

			} catch(...) {

				cerr << to_string() << "\tUnexpected error updating state" << endl;

			}

		}

		return *this;
	}

	bool User::Session::getProperty(const char *key, std::string &value) const noexcept {

		if(!strcasecmp(key,"username")) {
			value = to_string();
			return true;
		};

		if(!strcasecmp(key,"remote")) {
			value = remote() ? "true" : "false";
			return true;
		};

		if(!strcasecmp(key,"locked")) {
			value = locked() ? "true" : "false";
			return true;
		};

		if(!strcasecmp(key,"active")) {
			value = active() ? "true" : "false";
			return true;
		};

		return false;
	}

 }


 namespace std {

	UDJAT_API const char * to_string(const Udjat::User::State state) noexcept {

		if( ((size_t) state) >= (sizeof(statenames)/sizeof(statenames[0]))) {
			return _( "unknown" );
		}

#if defined(GETTEXT_PACKAGE)
		return dgettext(GETTEXT_PACKAGE,statenames[state]);
#else
		return statenames[state];
#endif

	}

 }

