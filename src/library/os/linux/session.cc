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
 #include <udjat/defs.h>
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
 #include <udjat/tools/quark.h>
 #include <udjat/tools/user/session.h>
 #include <udjat/tools/user/list.h>

 #ifdef HAVE_DBUS
	#include <udjat/tools/dbus/defs.h>
	#include <udjat/tools/dbus/connection.h>
	#include <udjat/tools/dbus/message.h>
 #endif // HAVE_DBUS

 using namespace std;

 namespace Udjat {

	User::Session::Session() {
		last_activity = time(0);
		User::List::getInstance().push_back(this);
	}

	User::Session::~Session() {
		User::List::getInstance().remove(this);
	}

	bool User::Session::remote() const {

		if(flags.remote == 0xFF) {

			// https://www.carta.tech/man-pages/man3/sd_session_is_remote.3.html
			int rc = sd_session_is_remote(sid.c_str());

			if(rc < 0) {
				error() << "sd_session_is_remote(" << sid << "): " << strerror(rc) << " (rc=" << rc << ")" << endl;
				return false;
			}

			User::Session * session = const_cast<User::Session *>(this);
			if(session) {
				session->flags.remote = rc > 0 ? 1 : 0;
			}

			return rc > 0;

		}

		return flags.remote != 0;
	}

	bool User::Session::active() const noexcept {

		int rc = sd_session_is_active(sid.c_str());
		if(rc < 0) {

			rc = -rc;
			if(rc == ENXIO) {
				info() << "sd_session_is_active(" << sid << "): " << strerror(rc) << " (rc=" << rc << "), assuming inactive" << endl;
			} else {
				error() << "sd_session_is_active(" << sid << "): " << strerror(rc) << " (rc=" << rc << ")" << endl;
			}

			return false;
		}

		return rc > 0;

	}

	const char * User::Session::path() const {

		if(dbpath.empty()) {

			// Dont have session path, get it and save for next calls.

			DBus::SystemBus::getInstance().call_and_wait(
				DBus::Message{
					"org.freedesktop.login1",
					"/org/freedesktop/login1",
					"org.freedesktop.login1.Manager",
					"GetSession",
					sid.c_str()
				},
				[&](DBus::Message & message) -> void {
					message.except();
					message.pop(const_cast<User::Session *>(this)->dbpath);
				}
			);
		}

		return dbpath.c_str();

	}

	bool User::Session::locked() const {

		bool locked = false;
		DBus::SystemBus::getInstance().call_and_wait(
			DBus::Message{
				"org.freedesktop.login1",
				this->path(),
				"org.freedesktop.DBus.Properties",
				"Get",
				"org.freedesktop.login1.Session",
				"LockedHint"
			},
			[&](DBus::Message & message) -> void {
				message.except();
				message.pop(locked);
				debug("Session is",(locked ? "locked" : "unlocked"));
			}
		);

		return locked;

	}

	bool User::Session::system() const {
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

	const char * User::Session::service() const {

		if(this->sname) {
			return this->sname;
		}

		//
		// Get session class name.
		//
		char *servicename = NULL;

		int rc = sd_session_get_service(sid.c_str(),&servicename);
		if(rc < 0 || !servicename) {
			rc = -rc;
			warning() << "sd_session_get_service(" << sid << "): " << strerror(rc) << " (rc=" << rc << "), assuming empty" << endl;
			return "";
		}

		const char *name = Quark{servicename}.c_str();
		free(servicename);

		User::Session * ses = const_cast<User::Session *>(this);
		if(ses) {
			ses->sname = name;
		}

		debug("Got service '",name,"' for session @",sid);
		return name;

	}

	const char * User::Session::classname() const noexcept {

		if(this->cname) {
			return this->cname;
		}

		//
		// Get session class name.
		//
		char *classname = NULL;

		int rc = sd_session_get_class(sid.c_str(),&classname);
		if(rc < 0 || !classname) {
			rc = -rc;
			warning() << "sd_session_get_class(" << sid << "): " << strerror(rc) << " (rc=" << rc << "), assuming empty" << endl;
			return "";
		}

		const char *name = Quark{classname}.c_str();
		free(classname);

		User::Session * ses = const_cast<User::Session *>(this);
		if(ses) {
			ses->cname = name;
		}

		debug("Got classname '",name,"' for session @",sid);
		return name;

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

	int User::Session::exec(const std::function<int()> &exec) const {
		return DBus::UserBus::exec(uid,exec);
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

