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
 #include <udjat/tools/object.h>
 #include <udjat/tools/expander.h>
 #include <udjat/tools/logger.h>

 using namespace Udjat;

 UserList::AlertProxy::AlertProxy(const pugi::xml_node &node, std::shared_ptr<Abstract::Alert> a)
		 : event(User::EventFactory(node)), alert(a) {

	const char *group = node.attribute("settings-from").as_string("alert-defaults");

#ifdef DEBUG
	cout << "alert\tAlert(" << alert->c_str() << ")= '" << event << "'" << endl;
#endif // DEBUG

	if(event & User::pulse) {

		emit.timer = Object::getAttribute(node,group,"interval",(unsigned int) 14400);
		if(!emit.timer) {
			throw runtime_error("Pulse alert requires the 'interval' attribute");
		}

	} else {

		emit.timer = 0;

	}

	//
	// 'old' style filters for compatibility.
	//
	emit.system = Object::getAttribute(node,group,"system-session",emit.system);
	emit.remote = Object::getAttribute(node,group,"remote-session",emit.remote);

	emit.background = Object::getAttribute(node,group,"background-session",emit.background);
	emit.foreground = Object::getAttribute(node,group,"foreground-session",emit.foreground);

	emit.locked = Object::getAttribute(node,group,"locked-session",emit.locked);
	emit.unlocked = Object::getAttribute(node,group,"unlocked-session",emit.unlocked);

	emit.active = Object::getAttribute(node,group,"active-session",emit.active);
	emit.inactive = Object::getAttribute(node,group,"inactive-session",emit.inactive);

	//
	// Filters
	//
	emit.system = Object::getAttribute(node,group,"allow-on-system-session",emit.system);
	emit.remote = Object::getAttribute(node,group,"allow-on-remote-session",emit.remote);

	emit.background = Object::getAttribute(node,group,"allow-on-background-session",emit.background);
	emit.foreground = Object::getAttribute(node,group,"allow-on-foreground-session",emit.foreground);

	emit.locked = Object::getAttribute(node,group,"allow-on-locked-session",emit.locked);
	emit.unlocked = Object::getAttribute(node,group,"allow-on-unlocked-session",emit.unlocked);

#ifndef _WIN32
	emit.classname = Object::getAttribute(node,group,"session-class",emit.classname);
	emit.service = Object::getAttribute(node,group,"session-service",emit.service);
#endif // !_WIN32

 }

 bool UserList::AlertProxy::test(const Udjat::User::Session &session) const noexcept {

	try {

		if(!emit.system && session.system()) {
			debug("rejecting ", session.name(), " by 'system' flag");
			return false;
		}

		if(!emit.remote && session.remote()) {
			debug("rejecting ", session.name(), " by 'remote' flag");
			return false;
		}

		if(session.active()) {

			if(!emit.active) {
				debug("rejecting ", session.name(), " by 'active' flag");
				return false;
			}

			bool locked = session.locked();

			if(!emit.locked && locked) {
				debug("rejecting ", session.name(), " by 'locked' flag");
				return false;
			}

			if(!emit.unlocked && !locked) {
				debug("rejecting ", session.name(), " by 'unlocked' flag");
				return false;
			}

		} else if(!emit.inactive) {

			debug("rejecting ", session.name(), " by 'inactive' flag");
			return false;

		}

#ifndef _WIN32
		if(emit.classname && *emit.classname && strcasecmp(emit.classname,session.classname().c_str())) {
			debug("rejecting ", session.name(), " by 'classname' flag");
			return false;
		}

		if(emit.service && *emit.service && strcasecmp(emit.service,session.service().c_str())) {
			debug("rejecting ", session.name(), " by 'service' flag");
			return false;
		}
#endif // !_WIN32

		return true;

	} catch(const std::exception &e) {

		session.error() << "Error checking alert flags: " << e.what() << endl;

	} catch(...) {

		session.error() << "Unexpected error checking alert flags" << endl;

	}

	return false;

 }


