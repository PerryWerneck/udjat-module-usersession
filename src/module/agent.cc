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

 using namespace std;
 using namespace Udjat;

 using Session = User::Session;

 UserList::Agent::Agent(const pugi::xml_node &node) : Abstract::Agent(node), controller(UserList::Controller::getInstance()) {
	load(node);
	controller->insert(this);
 }

 UserList::Agent::~Agent() {
	controller->remove(this);
 }

 void UserList::Agent::push_back(std::shared_ptr<Abstract::Alert> alert) {

 	const UserList::Alert * useralert = dynamic_cast<const UserList::Alert *>(alert.get());

 	if(useralert) {
		if(useralert->event == User::pulse) {
			auto timer = getUpdateInterval();
			if(!timer) {
				throw runtime_error("Agent 'update-timer' attribute is required to use 'pulse' alerts");
			}
			if(useralert->emit.timer < timer) {
				alert->warning() << "Pulse interval is lower than agent update timer" << endl;
			}
		}

 	}

	alerts.push_back(alert);
 }

 void UserList::Agent::emit(Udjat::Abstract::Alert &alert, Session &session) const noexcept {

	try {

#ifdef DEBUG
		cout << "** Emitting agent alert" << endl;
#endif // DEBUG

		auto activation = alert.ActivationFactory();
		activation->rename(session.name().c_str());
		activation->set(session);
		activation->set(*this);
		Udjat::start(activation);

	} catch(const std::exception &e) {

		error() << e.what() << endl;

	}

 }

 bool UserList::Agent::onEvent(Session &session, const Udjat::User::Event event) noexcept {

	bool activated = false;

	cout << session << "\t" << event << endl;

	for(auto alert : alerts) {

		UserList::Alert * useralert = dynamic_cast<UserList::Alert *>(alert.get());

		if(useralert && useralert->test(event) && useralert->test(session)) {

			activated = true;
			emit(*alert,session);

		}

	}

 	return activated;
 }

 bool UserList::Agent::refresh() {

	controller->User::Controller::for_each([this](shared_ptr<Udjat::User::Session> ses){

		Session * session = dynamic_cast<Session *>(ses.get());
		if(!session) {
			return;
		}

		// Get session idle time.
		time_t idletime = time(0) - session->alerttime();

		// Check pulse alerts against idle time.
#ifdef DEBUG
		cout << *session << "\tidle = " << idletime << endl;
#endif // DEBUG

		bool reset = false;
		for(auto alert : alerts) {

			UserList::Alert *useralert = dynamic_cast<UserList::Alert *>(alert.get());
			if(useralert) {

				auto timer = useralert->timer();

				if(timer && useralert->test(User::pulse) && useralert->test(*session)) {

					// Check for pulse.
					if(timer <= idletime) {
#ifdef DEBUG
						useralert->info() << "Emiting 'PULSE' " << (timer - idletime) << endl;
#endif // DEBUG
						reset |= true;
						emit(*alert,*session);
					}
#ifdef DEBUG
					else {
						useralert->info() << "Wait for " << (timer - idletime) << endl;
					}
#endif // DEBUG


				}


			}
		}

		if(reset) {
			session->reset();
		}

	});

	return false;
 }
