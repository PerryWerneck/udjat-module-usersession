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
 #include <udjat/tools/dbus/message.h>
 #include <udjat/tools/dbus/connection.h>
 #include <pthread.h>
 #include <sys/eventfd.h>
 #include <udjat/tools/threadpool.h>

 #include "private.h"

 #ifdef HAVE_DBUS
	#include <udjat/tools/dbus/connection.h>
 #endif // HAVE_DBUS

 #ifdef HAVE_UNISTD_H
	#include <unistd.h>
 #endif // HAVE_UNISTD_H

 using namespace std;

 namespace Udjat {

	/// @brief Find session (Requires an active guard!!!)
	User::Session & User::List::find(const char * sid) {

		lock_guard<recursive_mutex> lock(guard);
		for(auto session : sessions) {
			if(!strcmp(session->sid.c_str(),sid)) {
				return *session;
			}
		}

		// Not found, create a new one.
		Session * session = new Session();

		try {
			session->sid = sid;
			session->init();
		} catch(...) {
			delete session;
			throw;
		}

		return *session;
	}

	User::List::List() {
		debug("Starting user list");
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
			lock_guard<recursive_mutex> lock(guard);
			if(enabled) {
				throw runtime_error("Logind monitor is already active");
			}
			enabled = true;
		}

#ifdef HAVE_DBUS

		if(Config::Value<bool>("user-session","subscribe-prepare-for-sleep",true)) {

			try {
				DBus::SystemBus::getInstance().subscribe(
					"org.freedesktop.login1.Manager",
					"PrepareForSleep",
					[this](DBus::Message &message) {

						bool flag;
						message.pop(flag);

						if(flag) {
							sleep();
						} else {
							resume();
						}

						return false;

					}
				);
			} catch(const std::exception &e) {
				cerr << "users\tError '" << e.what() << "' subscribing to org.freedesktop.login1.Manager.PrepareForSleep" << endl;
			}
		}

		if(Config::Value<bool>("user-session","subscribe-prepare-for-shutdown",true)) {
			try {
				DBus::SystemBus::getInstance().subscribe(
					"org.freedesktop.login1.Manager",
					"PrepareForShutdown",
					[this](DBus::Message &message) {

						bool flag;
						message.pop(flag);

						if(flag) {
							shutdown();
						}

						return false;

					}
				);
			} catch(const std::exception &e) {
				cerr << "users\tError '" << e.what() << "' subscribing to org.freedesktop.login1.Manager.PrepareForShutdown" << endl;
			}
		}

		// https://www.freedesktop.org/software/systemd/man/latest/org.freedesktop.login1.html
		try {

			DBus::SystemBus::getInstance().subscribe(
				"org.freedesktop.login1.Manager",
				"SessionNew",
				[this](DBus::Message &message) {

					string sid;
					message.pop(sid);

					string path;
					message.pop(path);

					cout << "users\t Session '" << sid << "' started on path '" << path << "'" << endl;

					ThreadPool::getInstance().push([this,sid](){

						lock_guard<recursive_mutex> lock(guard);

						Session * session = new Session();

						try {

							session->sid = sid;
							session->init();

							session->flags.alive = true;
							session->emit(logon);

							char *state = nullptr;
							if(sd_session_get_state(sid.c_str(), &state) >= 0) {
								session->set(User::StateFactory(state));
								free(state);
							}

						} catch(const std::exception &e) {
							delete session;
							cerr << "users\tUnable to initialize session '" << sid << "': " << e.what() << endl;
						} catch(...) {
							delete session;
							cerr << "users\tUnable to initialize session '" << sid << "': Unexpected error" << endl;
						}

					});

					return false;

				}
			);
		} catch(const std::exception &e) {
			cerr << "users\tError '" << e.what() << "' subscribing to org.freedesktop.login1.Manager.SessionNew" << endl;
		}

		try {
			DBus::SystemBus::getInstance().subscribe(
				"org.freedesktop.login1.Manager",
				"SessionRemoved",
				[this](DBus::Message &message) {

					debug("----------------------------------> SessionRemoved");

					string sid;
					message.pop(sid);

					string path;
					message.pop(path);

					cout << "users\t Session '" << sid << "' finished on path '" << path << "'" << endl;

					ThreadPool::getInstance().push([this,sid](){

						lock_guard<recursive_mutex> lock(guard);

						for(Session *session : sessions) {

							if(strcmp(session->sid.c_str(),sid.c_str())) {
								continue;
							}

							if(session->flags.alive) {

								if(Logger::enabled(Logger::Debug)) {
									Logger::String{
									"Sid=",session->sid,
									" Uid=",session->userid(),
									" System=",session->system(),
									" type=",session->type(),
									" display=",session->display(),
									" remote=",session->remote(),
									" service=",session->service(),
									" class=",session->classname()
									}.write(Logger::Debug,session->name());
								}
								session->emit(logoff);
								session->flags.alive = false;
							}

							session->deinit();
							delete session;

							break;

						}

					});
					return false;

				}
			);
		} catch(const std::exception &e) {
			cerr << "users\tError '" << e.what() << "' subscribing to org.freedesktop.login1.Manager.SessionRemoved" << endl;
		}

		try {
			DBus::SystemBus::getInstance().subscribe(
				"org.freedesktop.login1.Manager",
				"UserNew",
				[this](DBus::Message &message) {

					debug("----------------------------------> UserNew");

					return false;

				}
			);
		} catch(const std::exception &e) {
			cerr << "users\tError '" << e.what() << "' subscribing to org.freedesktop.login1.Manager.UserNew" << endl;
		}

		try {
			DBus::SystemBus::getInstance().subscribe(
				"org.freedesktop.login1.Manager",
				"UserRemoved",
				[this](DBus::Message &message) {

					debug("----------------------------------> UserRemoved");

					return false;

				}
			);
		} catch(const std::exception &e) {
			cerr << "users\tError '" << e.what() << "' subscribing to org.freedesktop.login1.Manager.UserRemoved" << endl;
		}

#endif // HAVE_DBUS

		// Load active users
		ThreadPool::getInstance().push([this](){

			lock_guard<recursive_mutex> lock(guard);

			char **ids = nullptr;
			int idCount = sd_get_sessions(&ids);

			Logger::String{"Loading ",idCount," active users"}.info("users");

			for(int id = 0; id < idCount; id++) {

				try {

					Session *session = new Session();

					session->sid = ids[id];
					session->init();

					char *state = nullptr;
					if(sd_session_get_state(ids[id], &state) >= 0) {
						session->set(User::StateFactory(state));
						free(state);
					}

				} catch(const std::exception &e) {

					Logger::String{"Error '",e.what(),"' loading session '",ids[id],"'"}.error("users");

				}

				free(ids[id]);
			}

			free(ids);

			debug("------------------------------------> Will call init()");
			init();
			debug("------------------------------------> Returned from init()");

		});


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
			lock_guard<recursive_mutex> lock(guard);
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
