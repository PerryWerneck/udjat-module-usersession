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
 #include <systemd/sd-login.h>
 #include <systemd/sd-bus.h>
 #include <udjat/tools/configuration.h>
 #include <sys/types.h>
 #include <iostream>
 #include <unistd.h>
 #include <pwd.h>
 #include <functional>
 #include <sys/types.h>
 #include <unistd.h>
 #include <mutex>
 #include <udjat/tools/logger.h>

#ifdef HAVE_DBUS
	#include <udjat/tools/dbus.h>
#endif // HAVE_DBUS

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

	std::string User::Session::path() const {

		if(!dbpath.empty()) {
			return dbpath;
		}

/*
	dbus-send \
			--system \
			--dest=org.freedesktop.login1 \
			--print-reply \
			/org/freedesktop/login1 \
			org.freedesktop.login1.Manager.GetSession \
			string:1

*/
		sd_bus* bus = NULL;
		sd_bus_default_system(&bus);
		sd_bus_error error = SD_BUS_ERROR_NULL;
		sd_bus_message *reply = NULL;

		std::string response;

		try {

			int rc = sd_bus_call_method(
							bus,
							"org.freedesktop.login1",
							"/org/freedesktop/login1",
							"org.freedesktop.login1.Manager",
							"GetSession",
							&error,
							&reply,
							"s", sid.c_str()
						);

			if(rc < 0) {
				string message{error.message};
				sd_bus_error_free(&error);
				throw system_error(-rc,system_category(),message);
			} else if(!reply) {
				throw runtime_error("No reply from org.freedesktop.login1.Manager.GetSession");
			}

			const char *path = NULL;
			rc = sd_bus_message_read_basic(reply,SD_BUS_TYPE_OBJECT_PATH,&path);
			if(rc < 0) {
				sd_bus_message_unref(reply);
				throw system_error(-rc,system_category(),"org.freedesktop.login1.Manager.GetSession");

			}
			if(!(path && *path)) {
				sd_bus_message_unref(reply);
				throw runtime_error("Empty response from org.freedesktop.login1.Manager.GetSession");
			}

			response = path;
			trace() << "D-Bus Session path for @" << sid << " is " << response << endl;

			sd_bus_message_unref(reply);

			User::Session *session = const_cast<User::Session *>(this);
			if(session) {
				session->dbpath = response;
			}

		} catch(...) {

			sd_bus_unref(bus);
			throw;

		}

		sd_bus_unref(bus);

		return response;

	}

	bool User::Session::locked() const {

		if(!active()) {
			trace() << "Session @" << sid << " is not available for user input, 'locked' will be 'true'" << endl;
			return true;
		}

		int hint = 0;
		sd_bus* bus = NULL;
		sd_bus_error error = SD_BUS_ERROR_NULL;
		sd_bus_message *reply = NULL;

		sd_bus_default_system(&bus);

		try {

			int rc = sd_bus_call_method(
							bus,
							"org.freedesktop.login1",
							this->path().c_str(),
							"org.freedesktop.DBus.Properties",
							"Get",
							&error,
							&reply,
							"ss", "org.freedesktop.login1.Session", "LockedHint"
						);

			if(rc < 0) {
				throw system_error(-rc,system_category(),string{"org.freedesktop.login1.LockedHint: "} + error.message);
			} else if(!reply) {
				throw runtime_error("Empty response from org.freedesktop.login1.LockedHint");
			} else {

				// Get reply.
				if(sd_bus_message_read(reply,"v","b",&hint) < 0) {
					throw system_error(-rc,system_category(),"Can't read response from org.freedesktop.login1.LockedHint");
				}

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

		return hint != 0;

	}

	bool User::Session::system() const {
		if(this->uid != (uid_t) -1) {

		}

		return userid() < 1000;
	}

	std::string User::Session::display() const {

		char *display = NULL;

		int rc = sd_session_get_display(sid.c_str(),&display);
		if(rc < 0 || !display) {
			return "";
		}

		std::string str{display};
		free(display);
		return str;

	}

	std::string User::Session::type() const {

		char *type = NULL;

		int rc = sd_session_get_type(sid.c_str(),&type);
		if(rc < 0 || !type) {
			return "";
		}

		std::string str{type};
		free(type);
		return str;

	}

	std::string User::Session::service() const {

		char *service = NULL;

		int rc = sd_session_get_service(sid.c_str(),&service);
		if(rc < 0 || !service) {
			return "";
		}

		std::string str{service};
		free(service);
		return str;

	}

	std::string User::Session::classname() const {

		char *classname = NULL;

		int rc = sd_session_get_class(sid.c_str(),&classname);
		if(rc < 0 || !classname) {
			return "";
		}

		std::string str{classname};
		free(classname);
		return str;

	}

	int User::Session::userid() const {

		if(this->uid != (uid_t) -1) {
			return this->uid;
		}

		User::Session *session = const_cast<User::Session *>(this);

		int rc = sd_session_get_uid(session->sid.c_str(), &session->uid);

		if(rc < 0) {
			throw system_error(-rc,system_category(),string{"Cant get UID for session '"} + session->sid + "'");
		}

		return session->uid;
	}

	void User::Session::call(const std::function<void()> exec) {
		call(userid(),exec);
	}

	void User::Session::call(const uid_t uid, const std::function<void()> exec) {

		static mutex guard;
		lock_guard<mutex> lock(guard);

		uid_t saved_uid = geteuid();
		if(seteuid(uid) < 0) {
			throw std::system_error(errno, std::system_category(), "Cant set effective user id");
		}

		try {

			exec();

		} catch(...) {
			seteuid(saved_uid);
			throw;
		}

		seteuid(saved_uid);

	}

	const char * User::Session::name(bool update) const noexcept {

		if(update || username.empty()) {

			User::Session *session = const_cast<User::Session *>(this);
			if(!session) {
				cerr << "user\tconst_cast<> error" << endl;
				return "";
			}

			if(sd_session_get_uid(sid.c_str(), &session->uid)) {

				session->uid = -1;
				session->username = "@";
				session->username += sid;

			} else {

				int bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
				if (bufsize < 0)
					bufsize = 16384;

				string rc;
				char * buf = new char[bufsize];

				struct passwd     pwd;
				struct passwd   * result;
				if(getpwuid_r(uid, &pwd, buf, bufsize, &result)) {
					session->username = "@";
					session->username += sid;
				} else {
					session->username = buf;
				}
				delete[] buf;

			}

		}

		return username.c_str();

	}

 }

