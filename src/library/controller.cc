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
 #include <cstring>
 #include <iostream>

 using namespace std;

 namespace Udjat {

	void User::Controller::init() noexcept {

		lock_guard<mutex> lock(guard);
		for(auto session : sessions) {
#ifdef DEBUG
			cout << "users\tInitializing session @" << session->sid << endl;
#endif // DEBUG
			session->state.alive = true;
			session->onEvent(already_active);
		}

	}

	void User::Controller::deinit() noexcept {

		lock_guard<mutex> lock(guard);
		for(auto session : sessions) {
			if(session->state.alive) {
#ifdef DEBUG
				cout << "users\tDeinitializing session @" << session->sid << endl;
#endif // DEBUG
				session->onEvent(still_active);
				session->state.alive = false;
			}
		}

		sessions.clear();

	}

	std::shared_ptr<User::Session> User::Controller::SessionFactory() noexcept {
		// Default method, just create an empty session.
		return make_shared<User::Session>();
	}

	void User::Controller::for_each(std::function<void(std::shared_ptr<Session>)> callback) {
		lock_guard<mutex> lock(guard);
		for(auto session : sessions) {
			callback(session);
		}
	}

	void User::Controller::sleep() {
		cout << "users\tSystem is preparing to sleep" << endl;
		lock_guard<mutex> lock(guard);
		for(auto session : sessions) {
			session->onEvent(User::sleep);
		}
	}

	void User::Controller::resume() {
		cout << "users\tSystem is resuming from sleep" << endl;
		lock_guard<mutex> lock(guard);
		for(auto session : sessions) {
			session->onEvent(User::resume);
		}
	}

	void User::Controller::shutdown() {
		cout << "users\tSystem is preparing to shutdown" << endl;
		lock_guard<mutex> lock(guard);
		for(auto session : sessions) {
			session->onEvent(User::shutdown);
		}
	}

 }
