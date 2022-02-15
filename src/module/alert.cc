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
 #include <udjat/alert.h>
 #include <udjat/tools/expander.h>

 using namespace Udjat;

 inline Udjat::User::Event EventFactory(const pugi::xml_node &node) {
	return Udjat::User::EventFactory(
				node.attribute("event")
						.as_string(
							node.attribute("name").as_string()
						)
				);
 }

 UserList::Alert::Alert(const pugi::xml_node &node) : Udjat::Alert(node), event(EventFactory(node)) {

	const char *group = node.attribute("settings-from").as_string("alert-defaults");

#ifdef DEBUG
	cout << "alert\tAlert(" << c_str() << ")= '" << event << "'" << endl;
#endif // DEBUG

	if(event == User::pulse) {

		emit.timer = getAttribute(node,group,"interval",(unsigned int) 14400);
		if(!emit.timer) {
			throw runtime_error("Pulse alert requires the 'interval' attribute");
		}

	} else {

		emit.timer = 0;

	}

	emit.system = getAttribute(node,group,"on-system-session",emit.system);
	emit.remote = getAttribute(node,group,"on-remote-session",emit.remote);

	emit.background = getAttribute(node,group,"on-background-session",emit.background);
	emit.foreground = getAttribute(node,group,"on-foreground-session",emit.foreground);

	emit.locked = getAttribute(node,group,"on-locked-session",emit.locked);
	emit.unlocked = getAttribute(node,group,"on-unlocked-session",emit.unlocked);

 }

 UserList::Alert::~Alert() {
 }

 bool UserList::Alert::getProperty(const char *key, std::string &value) const noexcept {

	if(!strcasecmp(key,"eventname")) {
		value = std::to_string(event,false);
		return true;
	}

	return Udjat::Alert::getProperty(key,value);
 }

 bool UserList::Alert::test(const Udjat::User::Session &session) const noexcept {

	if(!emit.system && session.system()) {
		return false;
	}

	if(!emit.remote && session.remote()) {
		return false;
	}

	if(!emit.locked && session.locked()) {
		return false;
	}

	if(!emit.unlocked && !session.locked()) {
		return false;
	}

	return true;
 }

 std::shared_ptr<Abstract::Alert::Activation> UserList::Alert::ActivationFactory() const {
	return make_shared<Udjat::Alert::Activation>(this);
 }


