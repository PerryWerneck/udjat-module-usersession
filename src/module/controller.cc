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

 static const Udjat::ModuleInfo moduleinfo { "Users monitor" };

 UserList::Controller::Controller() : Udjat::Service("userlist",moduleinfo) {
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

 void UserList::Controller::for_each(std::function<void(UserList::Agent &agent)> callback) {
	lock_guard<mutex> lock(Udjat::Singleton::Container<UserList::Agent>::guard);
	for(auto agent : objects) {
		callback(*agent);
	}
 }
