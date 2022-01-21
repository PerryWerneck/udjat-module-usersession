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
 #include <udjat/module.h>
 #include <udjat/worker.h>
 #include <udjat/request.h>
 #include <udjat/alert.h>

 using namespace std;

 const Udjat::ModuleInfo UserList::info {
	PACKAGE_NAME,					// The module name.
	"Users list agent module",		// The module description.
	PACKAGE_VERSION, 				// The module version.
	PACKAGE_URL, 					// The package URL.
	PACKAGE_BUGREPORT 				// The bugreport address.
 };

 /// @brief Register udjat module.
 Udjat::Module * udjat_module_init() {

	class Module : public Udjat::Module, private Udjat::Worker {
	private:

		/// @brief Object factories.
		struct {
			UserList::Agent::Factory agent;
			UserList::Alert::Factory alert;
			Udjat::Alert::Factory urlalert;
		} factories;

	public:

		Module() : Udjat::Module("userlist",&UserList::info), Udjat::Worker("users",&UserList::info) {
		};

		virtual ~Module() {
		}

	};

	return new Module();
 }

