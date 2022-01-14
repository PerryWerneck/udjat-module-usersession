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

 #include "private.h"
 #include <udjat/tools/threadpool.h>

 using namespace std;

 mutex Controller::guard;

 std::shared_ptr<Controller> Controller::getInstance() {
	lock_guard<mutex> lock(guard);
	static std::shared_ptr<Controller> instance;
	if(!instance) {
		instance = make_shared<Controller>();
	}
	return instance;
 }

 std::shared_ptr<Udjat::User::Session> Controller::SessionFactory() noexcept {

	class Session : public Udjat::User::Session {
	protected:
		Udjat::User::Session & onEvent(const Udjat::User::Event &event) noexcept override {
			::Controller::getInstance()->for_each([this,event](UserList &agent){
				agent.onEvent(*this,event);
			});
			return *this;
		}

	public:
		Session() = default;

	};

	return make_shared<Session>();

 }


 void Controller::start() {
 	Udjat::User::Controller::load();
 }

 void Controller::stop() {
 	Udjat::User::Controller::unload();
 }

 void Controller::insert(UserList *agent) {
	lock_guard<mutex> lock(guard);
	agents.push_back(agent);
 }

 void Controller::remove(UserList *agent) {
	lock_guard<mutex> lock(guard);
	agents.remove_if([agent](const UserList *ag) {
		return ag == agent;
	});
 }

 void Controller::for_each(std::function<void(UserList &agent)> callback) {
	lock_guard<mutex> lock(guard);
	for(auto agent : agents) {
		callback(*agent);
	}
 }
