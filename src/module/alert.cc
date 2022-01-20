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

 UserList::Alert::Factory::Factory() : Udjat::Factory("user-action", &UserList::info) {
 }

 bool UserList::Alert::Factory::parse(Udjat::Abstract::Agent &parent, const pugi::xml_node &node) const {

	UserList::Agent *agent = dynamic_cast<UserList::Agent *>(&parent);

	if(!agent) {
		cerr << parent.getName() << "\tAn user-list agent is required for user-action alerts" << endl;
		return false;
	}

	agent->insert(make_shared<Alert>(node));
	return true;
 }


 inline Udjat::User::Event EventFromXmlNode(const pugi::xml_node &node) {
	return Udjat::User::EventFromName(
				node.attribute("event")
						.as_string(
							node.attribute("name").as_string()
						)
				);
 }

 UserList::Alert::Alert(const pugi::xml_node &node) : Udjat::Alert(node), event(EventFromXmlNode(node)) {

#ifdef DEBUG
	cout << "alert\tAlert '" << EventName(event) << "'" << endl;
#endif // DEBUG

 }

 UserList::Alert::~Alert() {
 }

