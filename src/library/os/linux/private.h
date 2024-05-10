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

 #pragma once

 #include <config.h>
 #include <udjat/defs.h>
 #include <udjat/tools/user/session.h>
 #include <udjat/tools/user/list.h>

 #ifdef HAVE_DBUS
	#include <udjat/tools/dbus/connection.h>
	#include <systemd/sd-bus.h>
 #endif // HAVE_DBUS

 namespace Udjat {

#ifdef HAVE_DBUS


	/// TODO: Replace for Udjat::DBus::SystemBus

	class SystemBus {
	private:
		sd_bus *connct = NULL;
		SystemBus();

	public:
		~SystemBus();
		static SystemBus & getInstance();
		int call_method(const char *destination, const char *path, const char *interface, const char *member, sd_bus_error *ret_error, sd_bus_message **reply, const char *types, ...);

	};

	struct BusMessage {
		sd_bus_message *ptr = NULL;
		~BusMessage() {
			debug("Unreferencing d-bus message");
			sd_bus_message_unrefp(&ptr);
		}
	};

	class User::Session::Bus : public Udjat::DBus::NamedBus {
	public:
		Bus(const char *busname, const char *name) : Udjat::DBus::NamedBus{busname,name} {
		}

	};
 #endif // HAVE_DBUS

 }


