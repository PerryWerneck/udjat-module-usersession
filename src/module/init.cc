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
 #include "private.h"
 #include <udjat/module.h>
 #include <udjat/worker.h>
 #include <udjat/request.h>
 #include <udjat/alerts/url.h>
 #include <udjat/moduleinfo.h>

 using namespace std;

 const Udjat::ModuleInfo UserList::info{"Users monitor"};

 /// @brief Register udjat module.
 Udjat::Module * udjat_module_init() {

	class Module : public Udjat::Module, private Udjat::Worker, private Udjat::Factory {
	private:

	protected:

		std::shared_ptr<Abstract::Agent> AgentFactory(const Abstract::Object UDJAT_UNUSED(&parent), const pugi::xml_node &node) const override {
			return make_shared<UserList::Agent>(node);
		}

		std::shared_ptr<Abstract::Alert> AlertFactory(const Abstract::Object UDJAT_UNUSED(&parent), const pugi::xml_node &node) const override {
#ifdef DEBUG
			Factory::info() << "Creating USER alert" << endl;
#endif // DEBUG
			return make_shared<UserList::Alert>(node);
		}

	public:

		Module() : Udjat::Module("users",UserList::info), Udjat::Worker("users",UserList::info), Udjat::Factory("users",UserList::info) {
		};

		virtual ~Module() {
		}

		bool get(Request UDJAT_UNUSED(&request), Response &response) const override {

			response.reset(Value::Array);

			auto sessions = UserList::Controller::getInstance();
			for(auto session : *sessions) {

				Value &row = response.append(Value::Object);

				row["name"] = session->to_string();
				row["remote"] = session->remote();
				row["locked"] = session->locked();
				row["active"] = session->active();
				row["state"] = std::to_string(session->state());

			}

			return true;
		}

	};

	return new Module();
 }

