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
 #include <udjat/tools/logger.h>

 using namespace std;
 using namespace Udjat;

 namespace UserList {

	Agent::Agent(const pugi::xml_node &node) : Abstract::Agent(node) {

		UserList::Controller::getInstance().push_back(this);

		if(!(properties.icon && *properties.icon)) {
			properties.icon = "user-info";
		}

		timers.max_pulse_check = getAttribute(node, "user-session", "max-update-timer", timers.max_pulse_check);

	}

	Agent::~Agent() {
		UserList::Controller::getInstance().remove(this);
	}

	Udjat::Value & Agent::get(Value &value) const {

		time_t idle = 0;
		UserList::Controller::getInstance().User::Controller::for_each([&idle](shared_ptr<Udjat::User::Session> user) {

			Session * usession = dynamic_cast<Session *>(user.get());
			if(usession && usession->alerttime()) {

				time_t uidle = (time(0) - usession->alerttime());

				if(uidle) {
					idle = std::max(idle,uidle);
				} else {
					idle = uidle;
				}

			}

		});

		value = idle;

		return value;
	}

	void Agent::emit(Udjat::Abstract::Alert &alert, Session &session) const noexcept {

		try {

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

		session.trace() << event << endl;

		for(AlertProxy &alert : proxies) {

			if(alert.test(event) && alert.test(session)) {

				// Emit alert.

				activated = true;

				alert.activate(*this,session);

				/*
				auto activation = alert.ActivationFactory();
				activation->set(session);
				activation->set(*this);

				Udjat::start(activation);
				*/

			}

		}

		return activated;
	}

	bool Agent::push_back(const pugi::xml_node &node, std::shared_ptr<Activatable> activatable){

		// First check if parent object can handle this activatable, if yes, just return.
		if(Abstract::Agent::push_back(node,activatable)) {
			return true;
		}

		// Parent was not able to handle this activatable, create a proxy for it.

		proxies.emplace_back(node,activatable);

		AlertProxy &proxy = proxies.back();

		if(proxy.test(User::pulse)) {

			auto timer = this->timer();

			if(!timer) {
				this->warning() << "Agent 'update-timer' attribute is required to use 'pulse' alerts" << endl;
				this->timer(14400);
			}

			if(proxy.timer() < timer) {
				activatable->warning() << "Pulse interval is lower than agent update timer" << endl;
				this->timer(timer);
			}

			Logger::String("Agent timer set to ",this->timer()).write(Logger::Debug,name());

		}

		return true;

	}

	bool Agent::getProperties(const char *path, Report &report) const {

		if(super::getProperties(path,report)) {
			return true;
		}

		if(*path) {
			return false;
		}

		report.start("username","state","locked","remote","system","display","type","service","class","activity","pulsetime",nullptr);

		UserList::Controller::getInstance().User::Controller::for_each([this,&report](shared_ptr<Udjat::User::Session> user) {

			report	<< user->name()
					<< user->state()
					<< user->locked()
					<< user->remote()
					<< user->system()
#ifdef _WIN32
					<< ""
					<< ""
					<< ""
					<< ""
#else
					<< user->display()
					<< user->type()
					<< user->service()
					<< user->classname()
#endif // _WIN32
					;

			time_t pulse = 0;

			UserList::Session * session = dynamic_cast<UserList::Session *>(user.get());

			if(session) {
				time_t alerttime = session->alerttime();
				report << TimeStamp(alerttime);

				if(alerttime) {
					for(const AlertProxy &alert : proxies) {
						time_t timer = alert.timer();
						time_t next = alerttime + timer;
						if(timer && alert.test(User::pulse) && alert.test(*session) && next > time(0)) {
							if(pulse) {
								pulse = std::min(pulse,next);
							} else {
								pulse = next;
							}
						}
					}
				}

			} else {
				report << "";
			}

			report << TimeStamp(pulse);

		});

		return true;
	}

	bool Agent::refresh() {

		Logger::String("Checking for updates").write(Logger::Debug,name());

		time_t required_wait = timers.max_pulse_check;
		UserList::Controller::getInstance().User::Controller::for_each([this,&required_wait](shared_ptr<Udjat::User::Session> ses) {

			Session * session = dynamic_cast<Session *>(ses.get());
			if(!session) {
				return;
			}

			// Get session idle time.
			time_t idletime = time(0) - session->alerttime();

			// Check pulse alerts against idle time.
			Logger::String("IDLE time is ",idletime).write(Logger::Debug,session->name());

			bool reset = false;
			for(AlertProxy &alert : proxies) {

				auto timer = alert.timer();

				if(timer && alert.test(User::pulse) && alert.test(*session)) {

					// Check for pulse.
					if(timer <= idletime) {

						debug("Emiting 'PULSE' for ",session->name()," idletime=",idletime," alert-timer=",alert.timer());

						reset |= true;

						alert.activate(*this,*session);

						/*
						auto activation = alert.ActivationFactory();

						activation->rename(name());
						activation->set(*this);
						activation->set(*session);

						Udjat::start(activation);
						*/

						required_wait = std::min(required_wait,timer);
						Logger::String("Will wait for ",timer," seconds").write(Logger::Debug,name());

					} else {

						time_t seconds{timer - idletime};
						required_wait = std::min(required_wait,seconds);
						Logger::String("Will wait for ",seconds," seconds").write(Logger::Debug,name());

					}

				}

			}

			if(reset) {
				session->reset();
			}

		});

		if(required_wait) {
			this->timer(required_wait);
			Logger::String("Next refresh set to ",TimeStamp(time(0)+required_wait)," (",required_wait," seconds)").write(Logger::Debug,name());
		}

		return false;
	}

	Value & Agent::getProperties(Value &value) const {
		super::getProperties(value);

		Udjat::Value &users = value["users"];

		UserList::Controller::getInstance().User::Controller::for_each([this,&users](shared_ptr<Udjat::User::Session> user) {
			user->getProperties(users.append(Udjat::Value::Object));
		});

		return value;
	}

	bool Agent::getProperties(const char *path, Value &value) const {

		if(super::getProperties(path,value)) {
			return true;
		}

		debug("Searching for user '",path,"'");
		for(auto user : UserList::Controller::getInstance()) {

			if(!strcasecmp(user->name(),path)) {
				user->getProperties(value);
				return true;
			}

		}

		return false;
	}

 }
