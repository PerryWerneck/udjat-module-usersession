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
 #include <udjat/tools/user/list.h>

 #include <cstring>
 #include <iostream>

 using namespace std;

 namespace Udjat {

	User::List & User::List::getInstance() {
		static User::List instance;
		return instance;
	}

	void User::List::init() noexcept {

		lock_guard<recursive_mutex> lock(guard);
		for(auto session : sessions) {
			cout << "users\tInitializing session @" << session->sid << endl;
			session->flags.alive = true;
			session->emit(already_active);
		}

	}

	void User::List::deinit() noexcept {

		lock_guard<recursive_mutex> lock(guard);

		while(!sessions.empty()) {

			Session *session = *sessions.begin();
			Logger::String{"Deinitializing session @",session->sid}.trace("userlist");
			if(session->flags.alive) {
				session->emit(still_active);
				session->flags.alive = false;
			}
			session->deinit();
			delete session;
		}

		/*
		for(auto session : sessions) {

			cout << *session << "\tDeinitializing session @" << session->sid << endl;

			if(session->flags.alive) {
				session->emit(still_active);
				session->flags.alive = false;
			}

			deinit(session);

		}

		sessions.clear();
		*/

	}

	void User::List::push_back(User::Agent *agent) {
		lock_guard<recursive_mutex> lock(guard);
		agents.push_back(agent);
	}

	void User::List::remove(User::Agent *agent) {
		lock_guard<recursive_mutex> lock(guard);
		agents.remove(agent);
	}

	void User::List::push_back(User::Session *session) {
		lock_guard<recursive_mutex> lock(guard);
		sessions.push_back(session);
	}

	void User::List::remove(User::Session *session) {
		lock_guard<recursive_mutex> lock(guard);
		sessions.remove(session);
	}

	bool User::List::for_each(const std::function<bool(Session &session)> &callback) {
		lock_guard<recursive_mutex> lock(guard);
		for(auto session : sessions) {
			if(callback(*session)) {
				return true;
			}
		}
		return false;
	}

	bool User::List::for_each(const std::function<bool(User::Agent &agent)> &callback) {
		lock_guard<recursive_mutex> lock(guard);
		for(User::Agent *agent : agents) {
			if(callback(*agent)) {
				return true;
			}
		}
		return false;
	}

	void User::List::sleep() {
		cout << "users\tSystem is preparing to sleep" << endl;
		lock_guard<recursive_mutex> lock(guard);
		for(auto session : sessions) {
			session->emit(User::sleep);
		}
	}

	void User::List::resume() {
		cout << "users\tSystem is resuming from sleep" << endl;
		lock_guard<recursive_mutex> lock(guard);
		for(auto session : sessions) {
			session->emit(User::resume);
		}
	}

	void User::List::shutdown() {
		cout << "users\tSystem is preparing to shutdown" << endl;
		lock_guard<recursive_mutex> lock(guard);
		for(auto session : sessions) {
			session->emit(User::shutdown);
		}
	}

 }
