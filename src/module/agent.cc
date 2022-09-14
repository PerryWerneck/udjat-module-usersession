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
 #include <udjat/agent/abstract.h>
 #include <udjat/alert/activation.h>
 #include <udjat/alert.h>
 #include <udjat/tools/threadpool.h>

 using namespace std;
 using namespace Udjat;

 // using Session = User::Session;
 // using Controller = UserList::Controller;

 namespace UserList {

	Agent::Agent(const pugi::xml_node &node) : Abstract::Agent(node) {
		UserList::Controller::getInstance().insert(this);
	}

	Agent::~Agent() {
		UserList::Controller::getInstance().remove(this);
	}

	void Agent::emit(Udjat::Abstract::Alert &alert, Session &session) const noexcept {

		try {

	#ifdef DEBUG
			cout << "** Emitting agent alert" << endl;
	#endif // DEBUG

			auto activation = alert.ActivationFactory();
			activation->rename(session.name());
			activation->set(session);
			activation->set(*this);
			Udjat::start(activation);

		} catch(const std::exception &e) {

			error() << e.what() << endl;

		}

	}

	bool Agent::onEvent(Session &session, const Udjat::User::Event event) noexcept {

		bool activated = false;

	#ifdef DEBUG
		cout << session << "\t" << __FILE__ << "(" << __LINE__ << ") " << event << " (" << alerts.size() << " alert(s))" << endl;
	#else
		cout << session << "\t" << event << endl;
	#endif // DEBUG

		for(AlertProxy &alert : alerts) {

			if(alert.test(event) && alert.test(session)) {

				// Emit alert.

				activated = true;

				auto activation = alert.ActivationFactory();
				activation->set(session);
				activation->set(*this);

				Udjat::start(activation);

			}

		}

		return activated;
	}

	void Agent::push_back(const pugi::xml_node &node, std::shared_ptr<Abstract::Alert> alert) {

		alerts.emplace_back(node,alert);

		AlertProxy &proxy = alerts.back();

		if(proxy.test(User::pulse)) {
			auto timer = this->timer();
			if(!timer) {
				throw runtime_error("Agent 'update-timer' attribute is required to use 'pulse' alerts");
			}
			if(proxy.timer() < timer) {
				alert->warning() << "Pulse interval is lower than agent update timer" << endl;
			}
		}

	}

	void Agent::get(const Request UDJAT_UNUSED(&request), Report &report) {

		report.start(name(),"username","state","locked","remote","system",nullptr);

		UserList::Controller::getInstance().User::Controller::for_each([&report](shared_ptr<Udjat::User::Session> user) {

			report	<< user->name()
					<< user->state()
					<< user->locked()
					<< user->remote()
					<< user->system();

		});

	}

	bool Agent::refresh() {

		UserList::Controller::getInstance().User::Controller::for_each([this](shared_ptr<Udjat::User::Session> ses) {

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
			for(AlertProxy &alert : alerts) {

				auto timer = alert.timer();

				if(timer && alert.test(User::pulse) && alert.test(*session)) {

					// Check for pulse.
					if(timer <= idletime) {

	#ifdef DEBUG
						info() << "Emiting 'PULSE' " << (timer - idletime) << endl;
	#endif // DEBUG
						reset |= true;

						auto activation = alert.ActivationFactory();

						activation->rename(name());
						activation->set(*this);
						activation->set(*session);

						Udjat::start(activation);
					}
	#ifdef DEBUG
					 else {
						info() << "Wait for " << (timer - idletime) << endl;
					}
	#endif // DEBUG


				}

			}

			if(reset) {
				session->reset();
			}

		});

		return false;
	}

 }
