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
 #include <udjat/moduleinfo.h>
 #include <udjat/version.h>
 #include <udjat/agent/user.h>
 #include <udjat/tools/user/list.h>

 using namespace std;

 /// @brief Register udjat module.
 Udjat::Module * udjat_module_init() {

	static const Udjat::ModuleInfo modinfo{"Users management module"};

	class Module : public Udjat::Module, private Udjat::Worker, private Udjat::Factory, private Udjat::Service {
	private:

	protected:

		std::shared_ptr<Abstract::Agent> AgentFactory(const Abstract::Object &, const XML::Node &node) const override {
			return make_shared<User::Agent>(node);
		}

	public:

		Module() : Udjat::Module("users",modinfo), Udjat::Worker("userlist",modinfo), Udjat::Factory("users",modinfo), Udjat::Service("userlist",modinfo) {
		};

		virtual ~Module() {
		}

		void start() override {
			User::List::getInstance().activate();
		}

		void stop() override {
			User::List::getInstance().deactivate();
		}

#if UDJAT_CHECK_VERSION(1,2,0)
		bool get(Request &, Response::Value &response) const override {

			response.reset(Value::Array);

			for(auto session : User::List::getInstance()) {

				Udjat::Value &row = response.append(Value::Object);

				row["name"] = session->to_string();
				row["remote"] = session->remote();
				row["locked"] = session->locked();
				row["active"] = session->active();
				row["state"] = std::to_string(session->state());

			}

			return true;
		}
#endif // UDJAT_CHECK_VERSION

	};

	return new Module();
 }

