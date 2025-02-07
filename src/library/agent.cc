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

#ifdef DEBUG
		Logger::String{"Event: '",std::to_string(event).c_str(),"' proxyes: ",proxies.size()}.info(session.name());
#endif
		
		time_t now = time(0);
		time_t seconds = 600;

		// Check for activation
		for(const auto &proxy : proxies) {

			if(proxy.timer) {
				time_t wait = proxy.timer - (now - session.activity());
				debug("---> Wait for ",wait);
				seconds = min(seconds,wait);
			}

			if(!(proxy.events & event)) {
				continue;
			}

			// Check event.
			debug(
				"Proxy '",proxy.activatable->name(),
				"' filter is '",std::to_string(proxy.filter).c_str(),
				"' test result was '",session.test(proxy.filter)
			);


			if(session.test(proxy.filter)
#ifndef _WIN32
				&& (!(proxy.classname && *proxy.classname) || strcasecmp(proxy.classname,session.classname()) == 0)
				&& (!(proxy.servicename && *proxy.servicename) || strcasecmp(proxy.servicename,session.service()) == 0)
#endif // !_WIN32
			) {
				Logger::String{
					"Activating event '",std::to_string(event),"' on session '",session.name(),"'"
				}.trace(proxy.activatable->name());
				activated = true;
				session.activity(now);
				proxy.activate(session,*this);

			} else if(Logger::enabled(Logger::Debug)) {

				Logger::String{
					"Ignoring event '",std::to_string(event),"' on session '",session.name(),"@",session.id().c_str(),"' due to filters"
				}.write(Logger::Debug,proxy.activatable->name());

				debug(
					"Session '",session.name(),"' rejected! (",
					(proxy.classname ? proxy.classname : "-"),"/",session.classname(),
					"  ",
					(proxy.servicename ? proxy.servicename : "-"),"/",session.service(),
					")"
				);

			}

		}

		timer(seconds);
		debug("Agent update set to ",TimeStamp(now+seconds).to_string()," (",seconds," seconds");

		return activated;

	}

/*
#ifdef DEBUG
template <typename I> 
inline std::string n2hexstr(I w, size_t hex_len = sizeof(I)<<1) {
    static const char* digits = "0123456789ABCDEF";
    std::string rc(hex_len,'0');
    for (size_t i=0, j=(hex_len-1)*4 ; i<hex_len; ++i,j-=4)
        rc[i] = digits[(w>>j) & 0x0f];
    return rc;
}
#endif 
*/

	bool User::Agent::push_back(const XML::Node &node, std::shared_ptr<Activatable> activatable){

		String trigger{node,"trigger-event"};
		if(trigger.empty()) {
			trigger = String{node,"name"};
		}

		auto event = User::EventFactory(trigger.c_str());
		if(!event) {
			// It's not an user event, call the default method.
			return super::push_back(node,activatable);
		}

		auto &proxy = proxies.emplace_back(node,event,activatable);

		debug("Filters: ",std::to_string(proxy.filter).c_str()," (",n2hexstr((uint16_t) proxy.filter).c_str(),")");

		if(event & User::pulse) {
			proxy.timer = XML::AttributeFactory(node,"interval").as_uint(proxy.timer);
			timer(min(timer(),proxy.timer));
		}

		return true;

	}

	bool User::Agent::refresh() {

		time_t seconds = 600;

		for(const auto &proxy : proxies) {

			if(proxy.events & User::pulse) {

				// Check 'pulse' event on all users.
				time_t now = time(0);
				User::List::getInstance().for_each([&](Udjat::User::Session &session) {

					if(!proxy.timer) {
						return false;
					}

					time_t idletime = now - session.activity();

					if(idletime >= proxy.timer) {

						if(
							session.test(proxy.filter)
#ifndef _WIN32
							&& (!(proxy.classname && *proxy.classname) || strcasecmp(proxy.classname,session.classname()) == 0)
							&& (!(proxy.servicename && *proxy.servicename) || strcasecmp(proxy.servicename,session.service()) == 0)
#endif // !_WIN32
						) {
							// Emit 'pulse' signal.
							Logger::String{"Emiting '",proxy.activatable->name(),"' action"}.trace(session.name());
							session.activity(now);
							proxy.activate(session,*this);
							seconds = min(seconds,proxy.timer);
						}

					} else {

						// How many seconds to this session becomes idle?
						time_t wait = proxy.timer - idletime;
						if(wait < seconds) {
							seconds = min(seconds,wait);
						}

					}

					return false;
				});
			}

		}

		debug("Sched update to ",seconds," seconds");
		timer(seconds);

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
