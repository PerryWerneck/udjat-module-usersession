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
 #include <udjat/tools/user/session.h>
 #include <udjat/tools/configuration.h>
 #include <systemd/sd-login.h>
 #include <udjat/tools/dbus/message.h>
 #include <iostream>
 #include <udjat/tools/threadpool.h>
 #include <udjat/tools/logger.h>

 #ifdef HAVE_DBUS
	#include <udjat/tools/dbus/connection.h>
 #endif // HAVE_DBUS

 using namespace std;

 namespace Udjat {

#ifdef HAVE_DBUS

	class UDJAT_PRIVATE User::Session::Bus : public DBus::UserBus {
	public:
		Bus(uid_t uid) : DBus::UserBus{uid} {
		}

	};

#endif // HAVE_DBUS

	void User::Session::init() {

		// Get UID (if available).
		if(sd_session_get_uid(sid.c_str(), &uid) < 0) {
			uid = -1;
		}

		// Store session class name.
		classname();

		// Store session remote state
		remote();

		// Log session info.
		Logger::String{
			"Sid=",sid,
			" Uid=",userid(),
			" System=",system(),
			" type=",type(),
			" display=",display(),
			" remote=",remote(),
			" service=",service(),
			" class=",classname()
		}.write(Logger::Debug,name());

		if(uid != -1 && !remote() && Config::Value<bool>("user-session","open-session-bus",true)) {

#ifdef HAVE_DBUS
			try {
				
				Logger::String{"Getting connection to user's bus"}.trace(to_string().c_str());
				userbus = make_shared<Bus>(uid);
			
				// Is the session locked?
				userbus->call(
					"org.gnome.ScreenSaver",
					"/org/gnome/ScreenSaver",
					"org.gnome.ScreenSaver",
					"GetActiveTime",
					[this](DBus::Message & message) {

						// Got an async d-bus response, check it.

						if(message) {

							unsigned int active;
							message.pop(active);

							if(active) {

								flags.locked = true;
								info() << "gnome-screensaver is active" << endl;

							} else {

								info() << "gnome-screensaver is not active" << endl;

							}

						} else {

							error() << "Error calling org.gnome.ScreenSaver.GetActiveTime: "  << message.error_message() << endl;

						}
					}
				);

				// Subscribe to gnome-screensaver
				// Hack to avoid gnome-screensaver lack of logind signal.

				// This would be far more easier with the fix of the issue
				// https://gitlab.gnome.org/GNOME/gnome-shell/-/issues/741#
				userbus->subscribe(
					"org.gnome.ScreenSaver",
					"ActiveChanged",
					[this](DBus::Message &message) {

						// Active state of gnome screensaver has changed, deal with it.
						bool locked;
						message.pop(locked);
						
						if(locked != flags.locked) {
							info() << "Gnome screensaver is now " << (locked ? "active" : "inactive") << endl;
							flags.locked = locked;
							ThreadPool::getInstance().push("user-lock-emission",[this,locked](){
								emit( (locked ? User::lock : User::unlock) );
							});
						}

						return false;
					}
				);

				// Another gnome signal from https://gitlab.gnome.org/GNOME/gnome-shell/-/blob/wip/jimmac/typography/data/dbus-interfaces/org.gnome.ScreenSaver.xml
				userbus->subscribe(
					"org.gnome.ScreenSaver",
					"WakeUpScreen",
					[this](DBus::Message &) {

						Logger::String{"Gnome screen saver WakeUpScreen signal"}.trace(name());

						return false;

					}
				);

			} catch(const std::exception &e) {

				userbus.reset();
				Logger::String{"Error talking to gnome-session: ",e.what()}.warning(to_string().c_str());

			}

#endif // HAVE_DBUS

		} else {

			Logger::String{"Not watching gnome screen-saver, disabled by configuration"}.trace(to_string().c_str());

		}

	}
 }
