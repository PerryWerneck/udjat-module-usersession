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

/// @brief Linux user controller.
///
/// References:
///
/// <http://www.matthew.ath.cx/misc/dbus>
/// <https://www.freedesktop.org/software/systemd/man/sd-login.html#>
///

// gdbus introspect --system --dest org.freedesktop.login1 --object-path /org/freedesktop/login1
// https://www.freedesktop.org/wiki/Software/systemd/logind/

// https://gitlab.gnome.org/GNOME/gnome-shell/-/issues/741

 #include <config.h>
 #include <udjat/tools/usersession.h>
 #include <systemd/sd-login.h>
 #include <cstring>
 #include <iostream>
 #include <poll.h>
 #include <signal.h>
 #include <udjat/tools/configuration.h>
 #include <udjat/tools/logger.h>
 #include <pthread.h>

#ifdef HAVE_DBUS
	#include <udjat/tools/dbus.h>
#endif // HAVE_DBUS

 using namespace std;

 namespace Udjat {

 	void User::Controller::refresh() noexcept {

		char **ids = nullptr;
		int idCount = sd_get_sessions(&ids);

#ifdef DEBUG
		cout << "users\tRefreshing " << idCount << " sessions" << endl;
#endif // DEBUG

		lock_guard<mutex> lock(guard);

		// Remove unused sessions.
		sessions.remove_if([ids,idCount](const shared_ptr<Session> &session) {

			for(int id = 0; id < idCount; id++) {
				if(!strcmp(session->sid.c_str(),ids[id]))
					return false;
			}

			// Reset states, just in case of some other one have an instance of this session.
			if(session->flags.alive) {
				session->emit(logoff);
				session->flags.alive = false;
			}
			return true;
		});

		// Create and update sessions.
		for(int id = 0; id < idCount; id++) {
			auto session = find(ids[id]);
			if(!session->flags.alive) {
				session->flags.alive = true;
				session->emit(logon);
			}

			char *state = nullptr;
			if(sd_session_get_state(ids[id], &state) >= 0) {
				session->set(User::StateFactory(state));
				free(state);
			}

			free(ids[id]);
		}

		free(ids);

	}

	/// @brief Find session (Requires an active guard!!!)
	std::shared_ptr<User::Session> User::Controller::find(const char * sid) {

		for(auto session : sessions) {
			if(!strcmp(session->sid.c_str(),sid)) {
				return session;
			}
		}

		// Not found, create a new one.
		std::shared_ptr<Session> session = SessionFactory();
		session->sid = sid;
		sessions.push_back(session);
		init(session);

		return session;
	}

	User::Controller::Controller() {
	}

	User::Controller::~Controller() {
		deactivate();
	}

	void User::Controller::activate() {

		{
			lock_guard<mutex> lock(guard);
			if(enabled) {
				throw runtime_error("Logind monitor is already active");
			}
			enabled = true;
		}

#ifdef HAVE_DBUS
		if(Config::Value<bool>("user-session","subscribe-prepare-for-sleep",true)) {
			try {
				DBus::Connection::getSystemInstance().subscribe(
					(void *) this,
					"org.freedesktop.login1.Manager",
					"PrepareForSleep",
					[this](DBus::Message &message) {

						if(DBus::Value(message).as_bool()) {
							sleep();
						} else {
							resume();
						}

					}
				);
			} catch(const std::exception &e) {
				cerr << "users\tError '" << e.what() << "' subscribing to org.freedesktop.login1.Manager.PrepareForSleep" << endl;
			}
		}

		if(Config::Value<bool>("user-session","subscribe-prepare-for-shutdown",true)) {
			try {
				DBus::Connection::getSystemInstance().subscribe(
					(void *) this,
					"org.freedesktop.login1.Manager",
					"PrepareForShutdown",
					[this](DBus::Message &message) {

						if(DBus::Value(message).as_bool()) {
							shutdown();
						}

					}
				);
			} catch(const std::exception &e) {
				cerr << "users\tError '" << e.what() << "' subscribing to org.freedesktop.login1.Manager.PrepareForShutdown" << endl;
			}
		}
#endif // HAVE_DBUS

		// Activate logind monitor.
		init();

		monitor = new std::thread([this](){

			pthread_setname_np(pthread_self(),"logind");

			Logger::trace() << "users\tlogind monitor is activating" << endl;

			{
				char **ids = nullptr;
				int idCount = sd_get_sessions(&ids);

				lock_guard<mutex> lock(guard);
				for(int id = 0; id < idCount; id++) {
					std::shared_ptr<Session> session = SessionFactory();
					session->sid = ids[id];
					sessions.push_back(session);
					init(session);

					char *state = nullptr;
					if(sd_session_get_state(ids[id], &state) >= 0) {
						session->set(User::StateFactory(state));
						free(state);
					}

					free(ids[id]);
				}

				free(ids);

			}

			init();

			sd_login_monitor * monitor = NULL;
			sd_login_monitor_new(NULL,&monitor);

			while(enabled) {
				struct pollfd pfd;
				memset(&pfd,0,sizeof(pfd));

				pfd.fd = sd_login_monitor_get_fd(monitor);
				pfd.events = sd_login_monitor_get_events(monitor) | SA_RESTART;
				pfd.revents = 0;

				uint64_t timeout_usec = 10;
				sd_login_monitor_get_timeout(monitor,&timeout_usec);
				if(timeout_usec > 1000)
					timeout_usec = 1000;

				int rcPoll = poll(&pfd,1,timeout_usec);
				switch(rcPoll) {
				case 0:	// Timeout.
					break;

				case -1: // Error!!
					if(errno != EINTR) {
						cerr << "users\tPoll error '" << strerror(errno) << "' on logind monitor, aborting" << endl;
						enabled = false;
					}
					break;

				default:	// Has event.
					if(pfd.revents) {
						sd_login_monitor_flush(monitor);
						refresh();
					}
				}
			}

			clog << "users\tlogind monitor is deactivating" << endl;

			deinit();

		});

		cout << "users\tLogind monitor is now active" << endl;

	}

	void User::Controller::deactivate() {

		{
			lock_guard<mutex> lock(guard);
			if(!enabled) {
				return;
			}
			enabled = false;
		}

#ifdef HAVE_DBUS
		DBus::Connection::getSystemInstance().unsubscribe(this);
#endif // HAVE_DBUS

		if(monitor) {

			cout << "users\tWaiting for termination of logind monitor" << endl;
			monitor->join();
			deinit();
			delete monitor;
			monitor = nullptr;
			cout << "users\tlogind monitor is now inactive" << endl;
		}

		deinit(); // Just in case.

	}


 }
