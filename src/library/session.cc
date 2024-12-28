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
 #include <udjat/tools/user/session.h>
 #include <iostream>
 #include <cstring>
 #include <udjat/tools/user/list.h>
 #include <udjat/agent/user.h>
 #include <udjat/tools/logger.h>

 using namespace std;

 static const char *statenames[] = {
	N_( "background" ),
	N_( "foreground" ),
	N_( "opening" ),
	N_( "closing" ),

	N_( "unknown" ),
 };

 namespace Udjat {

	User::State User::StateFactory(const char *statename) {

		debug("Searching for state '",statename,"'");

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

	std::string User::Session::to_string() const noexcept {
		if(username.empty()) {
			name(true);
		}
		return username;
	}

	User::Session & User::Session::set(User::State state) {

		if(state != this->flags.state) {

			cout	<< to_string()
					<< "\tState changes from '"
					<< this->flags.state
					<< "' to '"
					<< state << "' ("
					<< ((int) this->flags.state)
					<< "->"
					<< ((int) state)
					<< ")"
					<< endl;
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

	Udjat::Value & User::Session::getProperties(Udjat::Value &value) const {

		value["username"] = to_string();
		value["remote"] = remote();
		value["locked"] = locked();
		value["active"] = active();
#ifdef _WIN32
		value["display"] = "win32";
		value["type"] = "win32";
		value["service"] = "";
		value["classname"] = "";
		value["path"] = "";
		value["domain"] = domain();
#else
		value["display"] = display();
		value["type"] = type();
		value["service"] = service();
		value["classname"] = classname();
		value["path"] = path();
		value["domain"] = "";
#endif // !_WIN32

		return value;

	}

	bool User::Session::getProperty(const char *key, std::string &value) const {

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

		if(!strcasecmp(key,"display")) {
#ifndef _WIN32
			value = display();
#endif // !_WIN32
			return true;
		}

		if(!strcasecmp(key,"type")) {
#ifndef _WIN32
			value = type();
#endif // !_WIN32
			return true;
		}

		if(!strcasecmp(key,"service")) {
#ifndef _WIN32
			value = service();
#endif // !_WIN32
			return true;
		}

		if(!strcasecmp(key,"classname")) {
#ifndef _WIN32
			value = classname();
#endif // !_WIN32
			return true;
		}

		if(!strcasecmp(key,"path")) {
#ifndef _WIN32
			value = path();
#endif // !_WIN32
			return true;
		}

#ifdef _WIN32
		if(!strcasecmp(key,"domain")) {
			value = domain();
			return true;
		}
#endif // _WIN32

		return false;
	}

	const char * User::Session::name() const noexcept {
		return name(false);
	}

	bool User::Session::test(const Type type) const {

		if(type & (Locked|Unlocked)) {
			// Test lock type.
			if(!(type & (locked() ? Locked : Unlocked))) {
				debug("Rejecting session '",name(),"' by 'lock' flag");
				return false;
			}
		}

		if(type & (Background|Foreground)) {
			// Test foreground session.
			if(!(type & (foreground() ? Foreground : Background))) {
				debug("Rejecting session '",name(),"' by 'foreground' flag");
				return false;
			}
		}

		if(type & (System|User)) {
			// Test user type.
			if(!(type & (system() ? System : User))) {
				debug("Rejecting session '",name(),"' by 'system' flag");
				return false;
			}
		}

		if(type & (Active|Inactive)) {
			// Test Session state.
			if(!(type & (active() ? Active : Inactive))) {
				debug("Rejecting session '",name(),"' by 'active' flag");
				return false;
			}
		}

		if(type & (Remote|Local)) {
			// Test remote session.
			if(!(type & (remote() ? Remote : Local))) {
				debug("Rejecting session '",name(),"' by 'remote' flag");
				return false;
			}
		}

		debug("Allowing session ",name());
		return true;
	}

 	User::Session & User::Session::onEvent(const User::Event &event) noexcept {

		if(Logger::enabled(Logger::Trace)) {
			Logger::String{
				"--------> ",std::to_string(event)," sid=",this->sid,
				" alive=",alive(),
				" remote=",remote()
			}.trace(name());
		}

		/*
#ifdef DEBUG

		trace() << "session\t**EVENT** sid=" << this->sid << " Event=" << (int) event
				<< " Alive=" << (alive() ? "Yes" : "No")
				<< " Remote=" << (remote() ? "Yes" : "No")
				<< " User=" << to_string()
				<< endl;
#endif // DEBUG
		*/

		List::getInstance().for_each([this,event](User::Agent &ag){
			ag.onEvent(*this,event);
			return false;
		});

		return *this;
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

