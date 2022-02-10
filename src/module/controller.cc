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
 #include <udjat/tools/threadpool.h>
 #include <udjat/moduleinfo.h>

 using namespace std;

 mutex UserList::Controller::guard;

 std::shared_ptr<UserList::Controller> UserList::Controller::getInstance() {
	lock_guard<mutex> lock(guard);
	static std::shared_ptr<Controller> instance;
	if(!instance) {
		instance = make_shared<Controller>();
	}
	return instance;
 }

 static const Udjat::ModuleInfo moduleinfo { "Users monitor" };

 UserList::Controller::Controller() : Udjat::MainLoop::Service(moduleinfo) {
	service_name = "users";
 }

 std::shared_ptr<Udjat::User::Session> UserList::Controller::SessionFactory() noexcept {
	return make_shared<UserList::Session>();
 }

 void UserList::Controller::start() {
 	Udjat::User::Controller::activate();
 }

 void UserList::Controller::stop() {
 	Udjat::User::Controller::deactivate();
 }

 void UserList::Controller::insert(UserList::Agent *agent) {
	lock_guard<mutex> lock(guard);
	agents.push_back(agent);
 }

 void UserList::Controller::remove(UserList::Agent *agent) {
	lock_guard<mutex> lock(guard);
	agents.remove_if([agent](const UserList::Agent *ag) {
		return ag == agent;
	});
 }

 void UserList::Controller::for_each(std::function<void(UserList::Agent &agent)> callback) {
	lock_guard<mutex> lock(guard);
	for(auto agent : agents) {
		callback(*agent);
	}
 }
