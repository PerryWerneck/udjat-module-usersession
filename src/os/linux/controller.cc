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
 #include <cstring>
 #include <iostream>
 #include <poll.h>
 #include <signal.h>

 using namespace std;

 namespace Udjat {

	void User::Controller::refresh() noexcept {

		char **ids = nullptr;
		int idCount = sd_get_sessions(&ids);

		lock_guard<mutex> lock(guard);

		// Remove unused sessions.
		sessions.remove_if([ids,idCount](const shared_ptr<Session> &session) {

			for(int id = 0; id < idCount; id++) {
				if(!strcmp(session->sid.c_str(),ids[id]))
					return false;
			}

			// Reset states, just in case of some other one have an instance of this session.
			session->state.alive = session->state.remote = false;
			session->state.locked = true;
			return true;
		});

		// Create new sessions.
		for(int id = 0; id < idCount; id++) {
			find(ids[id])->state.alive = true;
		}

		// Cleanup
		for(int id = 0; id < idCount; id++)
			free(ids[id]);
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

		return session;
	}

	std::shared_ptr<User::Session> User::Controller::SessionFactory() noexcept {
		// Default method, just create an empty session.
		return make_shared<User::Session>();
	}

	User::Controller::Controller() {
	}

	User::Controller::~Controller() {

		std::thread *active = nullptr;
		{
			lock_guard<mutex> lock(guard);
			if(monitor) {
				clog << "users\tWaiting for deactivation of logind monitor" << endl;
				active = monitor;
				monitor = nullptr;
			}
		}

		if(active) {
			enabled = false;
			active->join();
			delete active;
		}

	}

	void User::Controller::start() {

		lock_guard<mutex> lock(guard);

		if(monitor) {
			throw runtime_error("Logind monitor is already active");
		}

		enabled = true;
		monitor = new std::thread([this](){

			clog << "users\tlogind monitor is activating" << endl;
			refresh();

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

			{
				// Finalize
				lock_guard<mutex> lock(guard);
				if(this->monitor) {
					this->monitor->detach();
					delete this->monitor;
					this->monitor = nullptr;
				}
			}

		});

	}

 }
