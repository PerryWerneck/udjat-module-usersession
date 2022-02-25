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
 #include <windows.h>
 #include <wtsapi32.h>
 #include <udjat/win32/exception.h>

 using namespace std;

 namespace Udjat {

	User::Session::Session() {
	}

	User::Session::~Session() {
	}

	bool User::Session::remote() const {
		return flags.remote;
	}

	bool User::Session::active() const {
		return flags.state == SessionInForeground;
	}

	bool User::Session::locked() const {
		return flags.locked;
	}

	bool User::Session::system() const {

		char	* name	= nullptr;
		DWORD	  szName;

		// https://docs.microsoft.com/en-us/windows/win32/api/wtsapi32/nf-wtsapi32-wtsquerysessioninformationa
		if(WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE,(DWORD) sid, WTSUserName,&name,&szName)) {
			bool rc = (name[0] < ' ');
			WTSFreeMemory(name);
			return rc;
		}

		cerr << "users\t" << Win32::Exception::format( (string{"Can't get username for sid @"} + std::to_string((int) sid)).c_str());
		return false;

	}

	std::string User::Session::to_string() const {

		char	* name	= nullptr;
		DWORD	  szName;

		if(WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE,(DWORD) sid, WTSUserName,&name,&szName) == 0) {
			cerr << "users\t" << Win32::Exception::format( (string{"Can't get username for sid @"} + std::to_string((int) sid)).c_str());
			return string{"@"} + std::to_string((int) sid);
		}

		if(name[0] < ' ') {
			WTSFreeMemory(name);
			return string{"@"} + std::to_string((int) sid);
		}

		string user((const char *) name);
		WTSFreeMemory(name);

		return user;

	}

 }

