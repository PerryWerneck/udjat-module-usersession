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
	return Udjat::User::EventFactory(
				node.attribute("event")
						.as_string(
							node.attribute("name").as_string()
						)
				);
 }

 UserList::Alert::Alert(const pugi::xml_node &node) : Udjat::Alert(node), event(EventFromXmlNode(node)) {

#ifdef DEBUG
	cout << "alert\tAlert(" << c_str() << ")= '" << EventName(event) << "'" << endl;
#endif // DEBUG

	system = node.attribute("emit-for-system-sessions").as_bool(false);
	remote = node.attribute("emit-for-remote-sessions").as_bool(false);

 }

 UserList::Alert::~Alert() {
 }

 std::shared_ptr<Abstract::Alert::Activation> UserList::Alert::ActivationFactory(const std::function<void(std::string &str)> &expander) const {

	class Activation : public Alert::Activation {
	public:
		Activation(const UserList::Alert &alert, const std::function<void(std::string &str)> &expander) : Alert::Activation(alert,expander) {
			string name{"${username}"};
			expander(name);
			this->name = name;
		}

	};

	return make_shared<Activation>(*this,expander);

 }

 void UserList::Alert::onEvent(shared_ptr<UserList::Alert> alert, const Udjat::User::Session &session, const Udjat::User::Event event) noexcept {

	if(event == alert->event) {

		if(session.system() && !alert->system) {
#ifdef DEBUG
			cout << session << "\tIgnoring system session" << endl;
#endif // DEBUG
			return;
		}

		if(session.remote() && !alert->remote) {
#ifdef DEBUG
			cout << session << "\tIgnoring remote session" << endl;
#endif // DEBUG
			return;
		}

#ifdef DEBUG
		cout << session << "\tEmitting alert to " << (session.system() ? "system" : "user" ) << " session" << endl;
		cout << session << "\tEmitting alert to " << (session.remote() ? "remote" : "local" ) << " session" << endl;
#endif // DEBUG

		Abstract::Alert::activate(alert,[&session,&event,alert](std::string &text) {

			Udjat::expand(text,[&session,&event,alert](const char *key, std::string &value){

				if(!strcasecmp(key,"username")) {
					value = session.to_string();
					return true;
				};

				if(!strcasecmp(key,"alertname")) {
					value = alert->name();
					return true;
				};

				if(!strcasecmp(key,"event")) {
					value = User::EventName(event);
					return true;
				};

				if(!strcasecmp(key,"activation")) {
					value = session.to_string();
					value += "/";
					value += User::EventName(event);
					return true;
				};

				return false;

			});

		});

	} else {

		alert->deactivate();

	}

 }




