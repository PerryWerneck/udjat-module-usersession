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

	}

	bool User::Agent::onEvent(Session &session, const Udjat::User::Event event) noexcept {

		bool activated = false;

		Logger::String{"Event: ",event}.info(session.name());
		time_t now = time(0);

		// Check for activation
		for(const auto &proxy : proxies) {

			// Check event.
			if((proxy.events & event) && session.test(proxy.filter)) {
				activated = true;
				session.activity(now);
				proxy.activate(session,*this);
			}

		}

		return activated;

	}

	bool User::Agent::push_back(const XML::Node &node, std::shared_ptr<Activatable> activatable){

		String trigger{node,"trigger-event"};
		if(trigger.empty()) {
			trigger = String{node,"name"};
		}

		auto event = EventFactory(trigger.c_str());
		if(!event) {
			// It's not an user event, call the default method.
			return super::push_back(node,activatable);
		}

		auto &proxy = proxies.emplace_back(event,Session::TypeFactory(node),activatable);

		if(event & User::pulse) {
			proxy.timer = XML::AttributeFactory(node,"interval").as_uint(proxy.timer);
			this->sched_update(5);
		}

		return true;

	}

	bool User::Agent::refresh() {

		time_t seconds = 60;

		for(const auto &proxy : proxies) {

			if(proxy.events & User::pulse) {

				// Check 'pulse' event on all users.
				time_t now = time(0);
				User::List::getInstance().for_each([&](Udjat::User::Session &session) {

					time_t idletime = now - session.activity();

					if(idletime >= proxy.timer) {

						if(session.test(proxy.filter)) {
							// Emit 'pulse' signal.
							session.activity(now);
							proxy.activate(session,*this);
						}

					} else {

						// How many seconds to this session becomes idle?
						time_t wait = proxy.timer - idletime;
						if(wait < seconds) {
							seconds = wait;
						}

					}

					return false;
				});
			}

		}

		this->sched_update(seconds);
		return true;

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
