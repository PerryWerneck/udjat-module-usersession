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

 UserList::Alert::Factory::Factory() : Udjat::Factory("user-action", UserList::info) {
 }

 bool UserList::Alert::Factory::parse(Udjat::Abstract::Agent &parent, const pugi::xml_node &node) const {

	UserList::Agent *agent = dynamic_cast<UserList::Agent *>(&parent);

	if(!agent) {
		parent.error() << "An user-list agent is required for user-action alerts" << endl;
		return false;
	}

	agent->insert(make_shared<Alert>(agent,node));
	return true;
 }


 inline Udjat::User::Event EventFactory(const pugi::xml_node &node) {
	return Udjat::User::EventFactory(
				node.attribute("event")
						.as_string(
							node.attribute("name").as_string()
						)
				);
 }

 UserList::Alert::Alert(const UserList::Agent *agent, const pugi::xml_node &node) : Udjat::Alert(node), event(EventFactory(node)) {

	const char *group = node.attribute("settings-from").as_string("alert-defaults");

#ifdef DEBUG
	cout << "alert\tAlert(" << c_str() << ")= '" << event << "'" << endl;
#endif // DEBUG

	if(event == User::pulse) {
		emit.timer = getAttribute(node,group,"interval",(unsigned int) 14400);
	} else {
		emit.timer = 0;
	}

	emit.system = getAttribute(node,group,"on-system-session",emit.system);
	emit.remote = getAttribute(node,group,"on-remote-session",emit.remote);

	emit.background = getAttribute(node,group,"on-background-session",emit.background);
	emit.foreground = getAttribute(node,group,"on-foreground-session",emit.foreground);

	emit.locked = getAttribute(node,group,"on-locked-session",emit.locked);
	emit.unlocked = getAttribute(node,group,"on-unlocked-session",emit.unlocked);

	if(event == User::pulse) {

		if(!agent->getUpdateInterval()) {
			throw runtime_error("'update-timer' attribute is required to use 'pulse' alerts");
		}

	}

 }

 UserList::Alert::~Alert() {
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

 std::shared_ptr<Abstract::Alert::Activation> UserList::Alert::ActivationFactory(const std::function<void(std::string &str)> &expander) const {

	class Activation : public Udjat::Alert::Activation {
	public:
		Activation(const Udjat::Alert &alert, const std::function<void(std::string &str)> &expander) : Udjat::Alert::Activation(alert,expander) {
			string name{"${username}"};
			expander(name);
			this->name = name;
		}

	};

	return make_shared<Activation>(*this,expander);

 }

 bool UserList::Alert::onEvent(shared_ptr<UserList::Alert> alert, const Udjat::User::Session &session, const Udjat::User::Event event) noexcept {

	if(event == alert->event && alert->test(session)) {

		Abstract::Alert::activate(alert,[&session,&event,alert](std::string &text) {

			Udjat::expand(text,[&session,&event,alert](const char *key, std::string &value){

				if(session.getProperty(key,value)) {
					return true;
				}

				if(!strcasecmp(key,"alertname")) {
					value = alert->name();
					return true;
				};

				if(!strcasecmp(key,"event")) {
					value = std::to_string(event,false);
					return true;
				};

				if(!strcasecmp(key,"description")) {
					value = std::to_string(event,true);
					return true;
				};

				if(!strcasecmp(key,"activation")) {
					value = session.to_string();
					value += "/";
					value += std::to_string(event);
					return true;
				};

				return false;

			});

		});

		return true;

	} else {

		alert->deactivate();

	}

	return false;

 }




