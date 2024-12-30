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
 #include <udjat/tools/intl.h>
 #include <udjat/tools/user/session.h>
 #include <udjat/tools/abstract/object.h>
 #include <udjat/alert.h>
 #include <udjat/agent/user.h>
 #include <udjat/tools/logger.h>

 static const struct {
	bool flag;
	const char *attrname;
 } typenames[] = {
	{ false,	"system"		},
	{ true,		"user"			},
	{ false,	"remote"		},
	{ true,		"local"			},
	{ false,	"locked"		},
	{ false,	"unlocked"		},
	{ false,	"background"	},
	{ false,	"foreground"	},
	{ false,	"active"		},
	{ false,	"inactive"		},
 };

 namespace std {

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

	string to_string(const Udjat::User::Session::Type type) {

		string rc;
		uint16_t mask = 0x0001;
		for(const auto &attr : typenames) {
			if(type & mask) {
				if(!rc.empty()) {
					rc += ",";
				}
				rc += attr.attrname;

			}
			mask <<= 1;
		}

		return rc;
	}

 }

 namespace Udjat {

	User::Session::Type User::Session::TypeFactory(const XML::Node &node) {

		uint16_t value = 0xFFFF;
		uint16_t mask = 0x01;
		
		for(const auto &type : typenames) {
			auto attr = XML::AttributeFactory(node,String{"allow-on-",type.attrname,"-session"}.c_str());
			if(!attr) {
				attr = XML::AttributeFactory(node,String{"on-",type.attrname,"-session"}.c_str());
			}
			if(!attr) {
				attr = XML::AttributeFactory(node,String{type.attrname,"-session"}.c_str());
			}
			if(attr.as_bool(type.flag)) {
				value |= mask;
			} else {
				value &= (~mask);
			}
			mask <<= 1;
		} 

		return (User::Session::Type) value;
	}

	User::Agent::Proxy::Proxy(const XML::Node &node, const User::Event e, std::shared_ptr<Activatable> a)
		: events{e},filter{User::Session::TypeFactory(node)},activatable{a} {

		if(dynamic_cast<Udjat::Alert *>(activatable.get())) {
			throw std::system_error(ENOTSUP, std::system_category(),"Unable to handle user based alerts");
		}

		classname = String{node,"session-class","user"}.as_quark();
		servicename = String{node,"session-service"}.as_quark();
						
	}

	void User::Agent::Proxy::activate(const User::Session &session, const Abstract::Object &agent) const noexcept {

		auto object = Abstract::Object::Factory(&session,&agent,nullptr);
		activatable->activate(*object);
		
	}

 }


/*

 #include <udjat/tools/object.h>
 #include <udjat/tools/logger.h>
 #include <udjat/tools/activatable.h>
 #include <udjat/alert/user.h>
 #include <udjat/agent/user.h>
 #include <iostream>

 using namespace Udjat;
 using namespace std;

 Udjat::User::Alert::Alert(const XML::Node &node, std::shared_ptr<Activatable> a)
		 : event{User::EventFactory(node)}, alert{a} {

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

	emit.locked = Object::getAttribute(node,group,"locked-session",event == User::Event::lock);
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

	emit.locked = Object::getAttribute(node,group,"allow-on-locked-session",event == User::Event::lock);
	emit.unlocked = Object::getAttribute(node,group,"allow-on-unlocked-session",emit.unlocked);

#ifndef _WIN32
	emit.classname = Object::getAttribute(node,group,"session-class",emit.classname);
	emit.service = Object::getAttribute(node,group,"session-service",emit.service);
#endif // !_WIN32

 }

 void Udjat::User::Alert::activate(const Agent &agent, const Session &session) {

	alert->activate([&agent,&session](const char *key, std::string &value){

		if(session.getProperty(key,value)) {
			return true;
		}

		if(agent.getProperty(key,value)) {
			return true;
		}

		return false;
	});

 }

 bool Udjat::User::Alert::test(const Udjat::User::Session &session) const noexcept {

	try {

		if(!emit.system && session.system()) {
			Logger::String{"Denying alert '",alert->name(),"' by 'system' flag"}.write(Logger::Debug,session.name());
			return false;
		}

		if(!emit.remote && session.remote()) {
			Logger::String{"Denying alert '",alert->name(),"' by 'remote' flag"}.write(Logger::Debug,session.name());
			return false;
		}

		if(session.active()) {

			if(!emit.active) {
				Logger::String{"Denying alert '",alert->name(),"' by 'active' flag"}.write(Logger::Debug,session.name());
				return false;
			}

			bool locked = session.locked();

			if(!emit.locked && locked) {
				Logger::String{"Denying alert '",alert->name(),"' by 'locked' flag"}.write(Logger::Debug,session.name());
				return false;
			}

			if(!emit.unlocked && !locked) {
				Logger::String{"Denying alert '",alert->name(),"' by 'unlocked' flag"}.write(Logger::Debug,session.name());
				return false;
			}

		} else if(!emit.inactive) {

			Logger::String{"Denying alert '",alert->name(),"' by 'inactive' flag"}.write(Logger::Debug,session.name());
			return false;

		}

#ifndef _WIN32
		if(emit.classname && *emit.classname && strcasecmp(emit.classname,session.classname())) {
			Logger::String{"Denying alert '",alert->name(),"' by 'classname' flag"}.write(Logger::Debug,session.name());
			return false;
		}

		if(emit.service && *emit.service && strcasecmp(emit.service,session.service())) {
			Logger::String{"Denying alert '",alert->name(),"' by 'service' flag"}.write(Logger::Debug,session.name());
			return false;
		}
#endif // !_WIN32

		Logger::String{"Allowing alert '",alert->name(),"'"}.write(Logger::Debug,session.name());
		return true;

	} catch(const std::exception &e) {

		session.error() << "Error checking alert '" << alert->name() << "' flags: " << e.what() << endl;

	} catch(...) {

		session.error() << "Unexpected error checking alert '" << alert->name() << "' flags" << endl;

	}

	return false;

 }
*/


