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
 #include <udjat/tools/user/session.h>
 #include <systemd/sd-login.h>
 #include <cstring>
 #include <iostream>
 #include <poll.h>
 #include <signal.h>
 #include <udjat/tools/configuration.h>
 #include <udjat/tools/logger.h>
 #include <pthread.h>
 #include <sys/eventfd.h>

 #include "private.h"

 #ifdef HAVE_DBUS
	#include <udjat/tools/dbus/connection.h>
 #endif // HAVE_DBUS

 #ifdef HAVE_UNISTD_H
	#include <unistd.h>
 #endif // HAVE_UNISTD_H

 using namespace std;

 namespace Udjat {

 	void User::List::refresh() noexcept {

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
				Logger::String(
					"Sid=",session->sid,
					" Uid=",session->userid(),
					" System=",session->system(),
					" type=",session->type(),
					" display=",session->display(),
					" remote=",session->remote(),
					" service=",session->service(),
					" class=",session->classname()
				).write(Logger::Debug,session->name());
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
	std::shared_ptr<User::Session> User::List::find(const char * sid) {

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

	User::List::List() {
		efd = eventfd(0,0);
		if(efd < 0) {
			Logger::String{"Error getting eventfd: ",strerror(errno)}.error("users");
		}
	}

	User::List::~List() {
		if(efd >= 0) {
			::close(efd);
		}
		deactivate();
	}

	void User::List::activate() {

		{
			lock_guard<mutex> lock(guard);
			if(enabled) {
				throw runtime_error("Logind monitor is already active");
			}
			enabled = true;
		}

#ifdef HAVE_DBUS

		try {

			if(!systembus) {
				systembus = make_shared<User::List::Bus>();
				cout << "Got system bus connection" << endl;
			}

		} catch(const std::exception &e) {

			cerr << "users\tError '" << e.what() << "' connecting to system bus" << endl;

		} catch(...) {

			cerr << "users\tUnexpected error connecting to system bus" << endl;

		}

		if(Config::Value<bool>("user-session","subscribe-prepare-for-sleep",true) && systembus) {

			try {
				systembus->subscribe(
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

		if(Config::Value<bool>("user-session","subscribe-prepare-for-shutdown",true) && systembus) {
			try {
				systembus->subscribe(
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

				struct pollfd pfd[2];
				memset(&pfd,0,sizeof(pfd));

				pfd[0].fd = sd_login_monitor_get_fd(monitor);
				pfd[0].events = sd_login_monitor_get_events(monitor) | SA_RESTART;
				pfd[0].revents = 0;
				pfd[1].fd = efd;
				pfd[1].events = POLLIN;
				pfd[1].revents = 0;

				uint64_t timeout_usec = 10;
				sd_login_monitor_get_timeout(monitor,&timeout_usec);

				if(efd < 0 && timeout_usec > 1000) {
					timeout_usec = 1000;
				}

				int rcPoll = poll(pfd, 2, timeout_usec);
				debug("rcPoll=",rcPoll);

				switch(rcPoll) {
				case 0:	// Timeout.
					debug("Timeout waiting for event");
					break;

				case -1: // Error!!
					if(errno != EINTR) {
						cerr << "users\tPoll error '" << strerror(errno) << "' on logind monitor, aborting" << endl;
						enabled = false;
					}
					break;

				default:	// Has event.
					if(pfd[0].revents) {
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

	void User::List::wakeup() {
		if(efd >= 0) {
			static uint64_t evNum = 1;
			debug("wake-up event ",evNum);
			if(write(efd, &evNum, sizeof(evNum)) != sizeof(evNum)) {
				Logger::String{"Error '",strerror(errno),"' writing to event loop using fd ",efd}.error("users");
			}
			evNum++;
		}
	}

	void User::List::deactivate() {

		debug(__FUNCTION__);

		{
			lock_guard<mutex> lock(guard);
			if(!enabled) {
				debug("User controller is not enabled");
				return;
			}
			enabled = false;
		}

		debug("Deactivating user controller");

		if(monitor) {

			debug("enabled: ",enabled);
			cout << "users\tWaiting for termination of logind monitor" << endl;

			wakeup();

			monitor->join();
			deinit();
			delete monitor;
			monitor = nullptr;
			cout << "users\tlogind monitor is now inactive" << endl;
		}

		deinit(); // Just in case.

	}


 }
