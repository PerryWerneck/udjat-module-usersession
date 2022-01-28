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
 #include "private.h"
 #include <udjat/tools/configuration.h>
 #include <iostream>
 #include <udjat/tools/threadpool.h>

 #ifdef HAVE_DBUS
	#include <udjat/tools/dbus.h>
 #endif // HAVE_DBUS

 using namespace std;

 namespace Udjat {

	void User::Controller::init(std::shared_ptr<Session> session) {

		if(Config::Value<bool>("user-session","open-session-bus",true)) {

#ifdef HAVE_DBUS
			try {

				string busname = session->getenv("DBUS_SESSION_BUS_ADDRESS");

				if(!busname.empty()) {

					// Connect to user's session bus.
					// Using session->call because you've to change the euid to
					// get access to the bus.
					session->call([session, &busname](){
						session->bus = (void *) new DBus::Connection(busname.c_str(),session->to_string().c_str());
					});

					// Is the session locked?
					((DBus::Connection *) session->bus)->call(
						"org.gnome.ScreenSaver",
						"/org/gnome/ScreenSaver",
						"org.gnome.ScreenSaver",
						"GetActiveTime",
						[session](DBus::Message & message) {

							// Got an async d-bus response, check it.

							if(message) {

								unsigned int active;
								message.pop(active);

								if(active) {

									session->state.locked = true;
									cout << *session << "\tgnome-screensaver is active" << endl;

								} else {

									cout << *session << "\tgnome-screensaver is not active" << endl;

								}

							} else {

								cerr << "Error '" << message.error_message() << "' calling org.gnome.ScreenSaver.GetActiveTime" << endl;

							}
						}
					);

					// Subscribe to gnome-screensaver
					// Hack to avoid gnome-screensaver lack of logind signal.

					// This would be far more easier with the fix of the issue
					// https://gitlab.gnome.org/GNOME/gnome-shell/-/issues/741#

					((DBus::Connection *) session->bus)->subscribe(
						(void *) session.get(),
						"org.gnome.ScreenSaver",
						"ActiveChanged",
						[session](DBus::Message &message) {

							// Active state of gnome screensaver has changed, deal with it.

							bool locked = DBus::Value(message).as_bool();
							if(locked != session->state.locked) {
								cout << *session << "\tSession was " << (locked ? "locked" : "unlocked") << " by gnome screensaver" << endl;
								session->state.locked = locked;
								ThreadPool::getInstance().push([session,locked](){
									session->onEvent( (locked ? User::lock : User::unlock) );
								});
							}

						}
					);

				}

			} catch(const exception &e) {

				cerr << *session << "\t" << e.what() << endl;
			}

#else

			clog << *session << "\tBuilt without Udjat::DBus, unable to watch gnome screensaver" << endl;

#endif // HAVE_DBUS

		}

	}
 }
