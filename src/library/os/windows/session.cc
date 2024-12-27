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

 #include <udjat/tools/user/session.h>
 #include <iostream>
 #include <windows.h>
 #include <wtsapi32.h>
 #include <udjat/win32/exception.h>
 #include <udjat/tools/quark.h>
 #include <udjat/tools/cleanup.h>
 #include <udjat/win32/cleanup.h>
 #include <udjat/tools/user/list.h>

 using namespace std;

 namespace Udjat {

	User::Session::Session() {
		last_activity = time(0):
		User::List::getInstance().push_back(this);
	}

	User::Session::~Session() {
		User::List::getInstance().remove(this);
	}

	bool User::Session::remote() const {
		return flags.remote;
	}

	bool User::Session::active() const noexcept {
		return flags.state == SessionInForeground;
	}

	bool User::Session::locked() const {
		return flags.locked;
	}

	std::string User::Session::domain() const {
		char	* name	= nullptr;
		DWORD	  szName;

		if(!WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE,(DWORD) sid, WTSDomainName,&name,&szName)) {
			warning() << "Can't get domain name: " << Win32::Exception::format() << endl;
			return "";
		}

		string dname{name};
		WTSFreeMemory(name);

		return dname;

	}

	bool User::Session::system() const {

		return flags.system;

		/*
		char	* name	= nullptr;
		DWORD	  szName;

		// https://docs.microsoft.com/en-us/windows/win32/api/wtsapi32/nf-wtsapi32-wtsquerysessioninformationa
		if(WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE,(DWORD) sid, WTSUserName,&name,&szName)) {
			bool rc = (name[0] < ' ');
			WTSFreeMemory(name);
			return rc;
		}

		cerr << "users\t" << Win32::Exception::format( (string{"Can't get usersystem state for sid @"} + std::to_string((int) sid)).c_str());
		return false;
		*/

	}

	static const char * UsernameFactory(DWORD sid) noexcept {
		string tempname{"@"};
		tempname += std::to_string((int) sid);
		return Quark(tempname).c_str();
	}

	const char * User::Session::name(bool update) const noexcept {

		if(update || username.empty()) {

			User::Session *session = const_cast<User::Session *>(this);
			if(!session) {
				cerr << "users\tconst_cast<> error getting username" << endl;
				return UsernameFactory(sid);
			}

			char * name	= nullptr;
			DWORD szName;

			if(WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE,(DWORD) sid, WTSUserName,&name,&szName) == 0) {

				cerr << "users\t" << Win32::Exception::format( (string{"Can't get username for sid @"} + std::to_string((int) sid)).c_str());
				session->flags.system = true;
				return UsernameFactory(sid);

			} else if(name[0] < ' ') {

				// cerr << "users\tUnexpected username for sid @" << sid << endl;
				session->flags.system = true;
				WTSFreeMemory(name);
				return UsernameFactory(sid);

			} else {

				session->flags.system = false;
				session->username = name;

			}

			WTSFreeMemory(name);

		}

		return username.c_str();

	}

 }

