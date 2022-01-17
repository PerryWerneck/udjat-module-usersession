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

 using namespace std;
 using namespace Udjat;

 UserList::UserList(const pugi::xml_node &node) : Abstract::Agent(node), controller(::Controller::getInstance()) {
	controller->insert(this);
	cout << getName() << "Users list created" << endl;
 }

 UserList::~UserList() {
	controller->remove(this);
	cout << getName() << "Users list destroyed" << endl;
 }

 void UserList::onEvent(Udjat::User::Session &session, const Udjat::User::Event &event) noexcept {

 	try {

		cout << getName()
			<< "\tname=" << session.to_string()
			<< " event=" << event
			<< " DBUS_SESSION_BUS_ADDRESS=" << session.getenv("DBUS_SESSION_BUS_ADDRESS")
			<< endl;

 	} catch(const std::exception &e) {
		cerr << getName() << "\t" << e.what() << endl;
 	}

 }
