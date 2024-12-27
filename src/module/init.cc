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
 #include <udjat/module/abstract.h>
 #include <udjat/agent/abstract.h>
 #include <udjat/tools/action.h>
 #include <udjat/tools/report.h>
 #include <udjat/moduleinfo.h>
 #include <udjat/agent/user.h>
 #include <udjat/tools/user/list.h>

 using namespace std;
 using namespace Udjat;

 /// @brief Register udjat module.
 Udjat::Module * udjat_module_init() {

	static const ModuleInfo modinfo{"Users management module"};

	class Module : public Udjat::Module, private Abstract::Agent::Factory, private Action::Factory {
	private:

	protected:

		std::shared_ptr<Abstract::Agent> AgentFactory(const Abstract::Object &, const XML::Node &node) const override {
			return make_shared<User::Agent>(node);
		}

	public:

		Module() 
			: Udjat::Module("users",modinfo), 
				Abstract::Agent::Factory("users"),
				Action::Factory("users") {

			// Get User list singleton.
			User::List::getInstance();

		};

		virtual ~Module() {
		}

		std::shared_ptr<Action> ActionFactory(const XML::Node &node) const {

			class UserListAction : public Udjat::Action {
			public:
				UserListAction(const XML::Node &node) : Udjat::Action{node} {
				}

				virtual ~UserListAction() {
				}

				int call(Udjat::Request &request, Udjat::Response &response, bool except) {
					return exec(response,except,[&](){

						auto &report = response.ReportFactory("name","remote","locked","active","state",nullptr);
						for(auto session : User::List::getInstance()) {
							report.push_back(session->to_string());
							report.push_back(session->remote());
							report.push_back(session->locked());
							report.push_back(session->active());
							report.push_back(std::to_string(session->state()));
						}

						return 0;
					});
				}

			};

			return make_shared<UserListAction>(node);
		}

	};

	return new Module();
 }

