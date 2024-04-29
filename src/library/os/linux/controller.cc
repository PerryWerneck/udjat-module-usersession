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

	/*
 	void User::List::refresh() noexcept {

		char **ids = nullptr;
		int idCount = sd_get_sessions(&ids);

#ifdef DEBUG
		cout << "users\tRefreshing " << idCount << " sessions" << endl;
#endif // DEBUG

		lock_guard<recursive_mutex> lock(guard);

		// Remove unused sessions.
		vector<Session *> deleted;

		for_each([&deleted,ids,idCount](Session &session){

			// Is the session in the list of active ones?
			for(int id = 0; id < idCount; id++) {
				if(!strcmp(session.sid.c_str(),ids[id])) {
					return false;
				}
			}

			// Cant find it in the list, mark for removal.
			deleted.push_back(&session);

			return false;
		});

		if(!deleted.empty()) {
			Logger::String{"Cleaning ",deleted.size()," unused session(s)"}.trace("Userlist");
			for(auto session : deleted) {

				// Reset states, just in case of some other one have an instance of this session.
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
			}
		}

		// Create and update sessions.
		for(int id = 0; id < idCount; id++) {

			try {

				debug("-------------------ID=",ids[id]);
				auto session = find(ids[id]);
				if(!session.flags.alive) {
					session.flags.alive = true;
					session.emit(logon);
				}

				char *state = nullptr;
				if(sd_session_get_state(ids[id], &state) >= 0) {
					session.set(User::StateFactory(state));
					free(state);
				}

			} catch(const std::exception &e) {

				Logger::String{e.what()}.error("userlist");

			}

			free(ids[id]);
		}

		free(ids);

	}
	*/

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

						return false;

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

						return false;

					}
				);
			} catch(const std::exception &e) {
				cerr << "users\tError '" << e.what() << "' subscribing to org.freedesktop.login1.Manager.PrepareForShutdown" << endl;
			}
		}

		if(systembus) {

			// https://www.freedesktop.org/software/systemd/man/latest/org.freedesktop.login1.html

			try {

				systembus->subscribe(
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
				systembus->subscribe(
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
				systembus->subscribe(
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
				systembus->subscribe(
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

		}
#endif // HAVE_DBUS

		debug("------------------------------------> Will call init()");
		init();
		debug("------------------------------------> Returned from init()");

		/*
		// Activate logind monitor.
		monitor = new std::thread([this](){

			pthread_setname_np(pthread_self(),"logind");

			Logger::String{"Monitor is activating"}.trace("logind");

			{
				char **ids = nullptr;
				int idCount = sd_get_sessions(&ids);

				lock_guard<recursive_mutex> lock(guard);
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

						Logger::String{e.what()}.error("userlist");

					}

					free(ids[id]);
				}

				free(ids);

			}

			init();

			sd_login_monitor * monitor = NULL;
			sd_login_monitor_new(NULL,&monitor);

			struct pollfd pfd[2];
			memset(&pfd,0,sizeof(pfd));
			pfd[0].fd = sd_login_monitor_get_fd(monitor);
			pfd[1].fd = efd;

			Logger::String{"Watching logind on socket ",pfd[0].fd}.trace("logind");

			while(enabled) {

				pfd[0].events = sd_login_monitor_get_events(monitor) | SA_RESTART;
				pfd[0].revents = 0;
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
					sd_login_monitor_flush(monitor);
					if(pfd[0].revents) {
						refresh();
					}
				}
			}

			Logger::String{"Monitor is deactivating"}.trace("logind");

			::close(pfd[0].fd);
			sd_login_monitor_unref(monitor);

			deinit();

		});
		cout << "users\tLogind monitor is now active" << endl;

		*/

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
