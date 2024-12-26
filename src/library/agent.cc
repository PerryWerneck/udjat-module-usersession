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
 #include <udjat/tools/string.h>


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

	static const struct {
		bool flag;
		const char *attrname;
	} session_types[] = {
		{ false,	"system"		},
		{ true,		"user"			},
		{ false,	"remote"		},
		{ true,		"locked"		},
		{ true,		"unlocked"		},
		{ true,		"background"	},
		{ true,		"foreground"	},
		{ true,		"active"		},
		{ true,		"inactive"		},
	};

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
			activated = true;
			proxy.activate(session,*this);

		}

		return activated;

	}

	bool User::Agent::push_back(const XML::Node &node, std::shared_ptr<Activatable> activatable){

		auto event = EventFactory(String{node,"trigger-event"}.c_str());
		if(!event) {
			// It's not an user event, call the default method.
			return super::push_back(node,activatable);
		}

		uint16_t filter = Proxy::All;
		uint16_t mask = 0x01;
		for(const auto &type : session_types) {
			auto attr = XML::AttributeFactory(node,String{"on-",type.attrname,"-session"}.c_str());
			if(!attr) {
				attr = XML::AttributeFactory(node,String{type.attrname,"-session"}.c_str());
			}
			if(attr.as_bool(type.flag)) {
				filter = (Proxy::Filter) (filter | mask);
			} else {
				filter = (Proxy::Filter) (filter & (~mask));
			}
			mask <<= 1;
		} 

		auto &proxy = proxies.emplace_back(event,filter,activatable);

		if(event & User::pulse && !this->timer()) {
			this->timer(14400);
			Logger::String{"update-timer attribute is required to use pulse alerts, setting it to ",this->timer()," seconds"}.warning(name());
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

		for(const auto &proxy : proxies) {

			if(!(proxy.events & User::pulse)) {
				continue;
			}

			// Emit 'pulse' event for all users.
			User::List::getInstance().for_each([&](Udjat::User::Session &session) {
				
				// FIXME: Should check session idle time.
				
				proxy.activate(session,*this);
				
				return false;
			});
		}
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
