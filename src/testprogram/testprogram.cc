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
 #include <udjat/tools/mainloop.h>
 #include <udjat/tools/usersession.h>
 #include <iostream>

#ifdef HAVE_SYSTEMD
	#include <systemd/sd-login.h>
#endif // HAVE_SYSTEMD

 using namespace std;
 using namespace Udjat;

 int main(int argc, const char **argv) {

	User::Controller userlist;

	userlist.start();

#ifdef HAVE_SYSTEMD
	MainLoop::getInstance().insert(NULL, 3000UL, [&userlist]() {

		for(auto session : userlist) {

			cout << session;

			try {
				cout
					<< "\tactive=" << (session->active() ? "yes" : "no")
					<< "\tlocked=" << (session->locked() ? "yes" : "no");
			} catch(const std::exception &e) {
				cout << endl;
				cerr << "\tError '" << e.what() << "'" << endl;
			}

			cout << endl;
		}

		return true;
	});
#endif // HAVE_SYSTEMD

	MainLoop::getInstance().run();
	userlist.stop();

	return 0;
 }
