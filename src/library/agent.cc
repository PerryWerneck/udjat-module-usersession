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
 #include <udjat/defs.h>
 #include <udjat/agent/abstract.h>
 #include <udjat/agent/user.h>
 #include <udjat/tools/user/list.h>
 #include <udjat/alert.h>


/*
 #include <udjat/tools/object.h>
 #include <udjat/agent/abstract.h>
 #include <udjat/alert.h>
 #include <udjat/tools/threadpool.h>
 #include <udjat/tools/logger.h>
 #include <udjat/alert/user.h>
 #include <udjat/tools/xml.h>
*/

 using namespace std;

 namespace Udjat {

	User::Agent::Agent(const XML::Node &node) : Abstract::Agent(node) {

		User::List::getInstance().push_back(this);

		if(!(properties.icon && *properties.icon)) {
			properties.icon = "user-info";
		}

		timers.max_pulse_check = getAttribute(node, "user-session", "max-update-timer", timers.max_pulse_check);

	}

	User::Agent::~Agent() {
		User::List::getInstance().remove(this);
	}

	time_t User::Agent::get() const noexcept {
		return time(0)-alert_timestamp;
	}

	Udjat::Value & User::Agent::get(Value &value) const {
		value = this->get();
		return value;
	}

	void User::Agent::emit(Udjat::Activatable &activatable, Session &session) const noexcept {

		class Object : public Udjat::Abstract::Object {
		private:
			const Udjat::Abstract::Object &session;
			const Udjat::Abstract::Object &agent;

		public:
			Object(const Udjat::Abstract::Object &s, const Udjat::Abstract::Object &a)
				: session{s}, agent{a} {
			}

			virtual ~Object() {				
			}

			bool getProperty(const char *key, std::string &value) const override {
				if(session.getProperty(key,value)) {
					return true;
				}
				return agent.getProperty(key,value);
			}

		};

		try {
			
			activatable.activate(Object{session,*this});

		} catch(const std::exception &e) {

			error() << e.what() << endl;

		}

		/*
		try {

			auto activation = alert.ActivationFactory();
			activation->rename(session.name());
			activation->set(session);
			activation->set(*this);
			Udjat::start(activation);

		} catch(const std::exception &e) {

			error() << e.what() << endl;

		}
		*/

	}

	bool User::Agent::onEvent(Session &session, const Udjat::User::Event event) noexcept {

		bool activated = false;

		session.info() << "Event: " << event << endl;

		// Check proxies
		for(const auto &proxy : proxies) {

			// Check event.
			if(!(proxy.events & event)) {
				continue;
			}

			// Check session state.

			#error Parei aqui

		}

		if(activated) {
			alert_timestamp = time(0);
			sched_update(timer()); // Reset timer for pulse event.
		}

		return activated;
	}

	bool User::Agent::push_back(const XML::Node &node, std::shared_ptr<Activatable> activatable){

		// First check if parent object can handle this activatable, if yes, just return.
		if(Abstract::Agent::push_back(node,activatable)) {
			return true;
		}

		// Parent was not able to handle this activatable, create a proxy for it.

		proxies.emplace_back(node,activatable);

		User::Alert &proxy = proxies.back();

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

	/*
	bool User::Agent::getProperties(const char *path, Report &report) const {

		if(super::getProperties(path,report)) {
			return true;
		}

		if(*path) {
			return false;
		}

		report.start("username","state","locked","remote","system","domain","display","type","service","class","activity","pulsetime",nullptr);

		User::List::getInstance().for_each([this,&report](Udjat::User::Session &user) {

			report.push_back(user.name());

			report.push_back(user.state());
			report.push_back(user.locked());
			report.push_back(user.remote());
			report.push_back(user.system());
#ifdef _WIN32
			report.push_back(user.domain());
			report.push_back(""); // display
			report.push_back(""); // type
			report.push_back(""); // service
			report.push_back(""); // classname
#else
			report.push_back(""); // domain
			report.push_back(user.display());
			report.push_back(user.type());
			report.push_back(user.service());
			report.push_back(user.classname());
#endif // _WIN32

			FIXME: Get pulse time
			{
				time_t pulse = 0;

				for(const User::Alert &alert : proxies) {
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

				if(pulse) {
					report << TimeStamp(pulse);
				} else {
					report << "";
				}

			}
			report.push_back("");
			return false;

		});

		return true;
	}
	*/

	bool User::Agent::refresh() {

#ifdef DEBUG
		Logger::String("Checking for updates").write(Logger::Debug,name());
#endif // DEBUG

		time_t required_wait = timers.max_pulse_check;
		User::List::getInstance().for_each([this,&required_wait](Udjat::User::Session &session) {

			time_t idletime = time(0) - alert_timestamp;	// Get time since last alert.

			for(User::Alert &alert : proxies) {

				auto timer = alert.timer();	// Get alert timer.

				if(timer && alert.test(User::pulse) && alert.test(session)) {

					// Check for pulse.
					if(timer <= idletime) {

						Logger::String{"Emitting PULSE (idletime=",idletime," alert-timer=",alert.timer(),")"}.write(Logger::Debug,name());

						alert.activate(*this,session);

						required_wait = std::min(required_wait,timer);
						Logger::String{"Will wait for ",timer," seconds"}.write(Logger::Debug,name());

					} else {

						time_t seconds{timer - idletime};
						required_wait = std::min(required_wait,seconds);
						Logger::String{"Will wait for ",seconds," seconds"}.write(Logger::Debug,name());

					}

				}

			}

			return false;
		});

		if(required_wait) {
			this->timer(required_wait);
			Logger::String{"Next refresh set to ",TimeStamp(time(0)+required_wait)," (",required_wait," seconds)"}.write(Logger::Debug,name());
		}

		return false;
	}

	Value & User::Agent::getProperties(Value &value) const {
		super::getProperties(value);

		Udjat::Value &users = value["users"];

		User::List::getInstance().for_each([this,&users](Udjat::User::Session &user) {
			user.getProperties(users.append(Udjat::Value::Object));
			return false;
		});

		return value;
	}

	bool User::Agent::getProperties(const char *path, Value &value) const {

		if(super::getProperties(path,value)) {
			return true;
		}

		debug("Searching for user '",path,"'");
		for(auto user : User::List::getInstance()) {

			if(!strcasecmp(user->name(),path)) {
				user->getProperties(value);
				return true;
			}

		}

		return false;
	}

 }
