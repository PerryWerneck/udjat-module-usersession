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
 #include <systemd/sd-login.h>
 #include <sys/types.h>
 #include <iostream>
 #include <unistd.h>
 #include <pwd.h>

 using namespace std;

 namespace Udjat {

	User::Session::Session() {
	}

	User::Session::~Session() {
	}

	bool User::Session::remote() const {
		// https://www.carta.tech/man-pages/man3/sd_session_is_remote.3.html
		return (sd_session_is_remote(sid.c_str()) > 0);
	}

	bool User::Session::locked() const {
		throw system_error(ENOTSUP,system_category(),"Lock status is not implemented");
	}

	std::string User::Session::to_string() const noexcept {

		uid_t uid = (uid_t) -1;

		if(sd_session_get_uid(sid.c_str(), &uid)) {
			string rc{"@"};
			return rc + sid;
		}

		int bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
		if (bufsize < 0)
			bufsize = 16384;

		string rc;
		char * buf = new char[bufsize];

		struct passwd     pwd;
		struct passwd   * result;
		if(getpwuid_r(uid, &pwd, buf, bufsize, &result)) {
			rc = "@";
			rc += sid;
		} else {
			rc = buf;
		}
		delete[] buf;

		return rc;

	}

 }

