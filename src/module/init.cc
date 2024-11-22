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
 #include <udjat/tools/interface.h>
 #include <udjat/tools/report.h>
 #include <udjat/moduleinfo.h>
 #include <udjat/version.h>
 #include <udjat/agent/user.h>
 #include <udjat/tools/user/list.h>

 using namespace std;

 /// @brief Register udjat module.
 Udjat::Module * udjat_module_init() {

	static const Udjat::ModuleInfo modinfo{"Users management module"};

	class Module : public Udjat::Module, private Udjat::Interface, private Udjat::Factory, private Udjat::Service {
	private:

	protected:

		std::shared_ptr<Abstract::Agent> AgentFactory(const Abstract::Object &, const XML::Node &node) const override {
			return make_shared<User::Agent>(node);
		}

	public:

		Module() : Udjat::Module("users",modinfo), Udjat::Interface("userlist"), Udjat::Factory("users",modinfo), Udjat::Service("userlist",modinfo) {
		};

		virtual ~Module() {
		}

		void start() override {
			User::List::getInstance().activate();
		}

		void stop() override {
			User::List::getInstance().deactivate();
		}

        bool for_each(const std::function<bool(const size_t index, bool input, const char *name, const Value::Type type)> &call) const override {

			static const char *names[] = {
				"name",
				"remote",
				"locked",
				"active",
				"state",
			};

			for(size_t ix = 0; ix < N_ELEMENTS(names); ix++) {
				if(call(ix,false,names[ix],Value::String)) {
					return true;
				}
			}

			return false;
		}

		void call(const char *path, Udjat::Value &response) override {

			auto &report = response.ReportFactory("name","remote","locked","active","state",nullptr);
			for(auto session : User::List::getInstance()) {
				report.push_back(session->to_string());
				report.push_back(session->remote());
				report.push_back(session->locked());
				report.push_back(session->active());
				report.push_back(std::to_string(session->state()));
			}

		}

	};

	return new Module();
 }

