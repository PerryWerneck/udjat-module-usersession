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
 #include <systemd/sd-bus.h>
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

	bool User::Session::active() const {

		int rc = sd_session_is_active(sid.c_str());
		if(rc < 0) {
			throw system_error(-rc,system_category(),"sd_session_is_active");
		}

		return rc > 0;

	}

	bool User::Session::locked() const {

		sd_bus* bus = NULL;
		sd_bus_error error = SD_BUS_ERROR_NULL;
		sd_bus_message *reply = NULL;

		sd_bus_default_system(&bus);

		try {

/*
	dbus-send \
			--system \
			--dest=org.freedesktop.login1 \
			--print-reply \
			/org/freedesktop/login1/session/_31 \
			org.freedesktop.DBus.Properties.Get \
			string:org.freedesktop.login1.Session string:LockedHint
*/
			int rc = sd_bus_call_method(
							bus,
							"org.freedesktop.login1",
							"/org/freedesktop/login1/session/_31",
							"org.freedesktop.DBus.Properties",
							"Get",
							&error,
							&reply,
							"ss", "org.freedesktop.login1.Session", "LockedHint"
						);


			/*
			int rc = sd_bus_get_property(
				bus,
				"org.freedesktop.login1",
				"/org/freedesktop/login1/session/_31",
				"org.freedesktop.DBus.Properties",
				"LockedHint",
				&error,
				&reply,
				NULL
			);
			*/

			if(rc < 0) {
				throw system_error(-rc,system_category(),error.message);
			} else if(!reply) {
				throw runtime_error("Empty response from org.freedesktop.DBus.Properties.LockedHint");
			} else {

				// Get reply.

			}

		} catch(...) {
			if(reply) {
				sd_bus_message_unref(reply);
			}
			sd_bus_error_free(&error);
			sd_bus_unref(bus);
			throw;
		}

		sd_bus_error_free(&error);
		if(reply) {
			sd_bus_message_unref(reply);
		}
		sd_bus_unref(bus);


		return false; // FIX-ME

/*

	sd_bus_call_method

	DBusGMainLoop(set_as_default=True)                        # integrate into gobject main loop
	bus = dbus.SystemBus()                                    # connect to system wide dbus
	bus.add_signal_receiver(                                  # define the signal to listen to
		locker_callback,                                      # callback function
		'LockedHint',                                         # signal name
		'org.freedesktop.DBus.Properties.PropertiesChanged',  # interface
		'org.freedesktop.login1'                              # bus name
	)

	https://dbus.freedesktop.org/doc/dbus-java/api/org/freedesktop/DBus.Properties.html

	https://cpp.hotexamples.com/examples/-/-/sd_bus_get_property_string/cpp-sd_bus_get_property_string-function-examples.html


	int sd_bus_get_property(
			sd_bus *bus,
			const char *destination,	"org.freedesktop.login1"
			const char *path,			"/org/freedesktop/login1/session/_31"
			const char *interface,		"org.freedesktop.DBus.Properties"
			const char *member, 		"LockedHint"
			sd_bus_error *ret_error,
			sd_bus_message **reply,
			const char *type
		);

sd_bus_call_method(bus,
                        "org.freedesktop.login1"
                        "/org/freedesktop/login1/session/_31",
                        "org.freedesktop.DBus.Properties",
                        "Get",
                        &error,
                        &reply,
                        "ss", "org.freedesktop.login1", "LockedHint");


	busctl tree org.freedesktop.login1

	dbus-send \
			--system \
			--dest=org.freedesktop.login1 \
			--print-reply \
			/org/freedesktop/login1/session/_31 \
			org.freedesktop.DBus.Properties.Get \
			string:org.freedesktop.login1.Session string:LockedHint

*/
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

