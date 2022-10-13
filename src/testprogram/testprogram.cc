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

 #include <udjat/tools/systemservice.h>
 #include <udjat/tools/application.h>
 #include <udjat/agent.h>
 #include <udjat/factory.h>
 #include <udjat/module.h>
 #include <iostream>
 #include <memory>

 using namespace std;
 using namespace Udjat;

//---[ Implement ]------------------------------------------------------------------------------------------

int main(int argc, char **argv) {

	class Service : public SystemService {
	protected:
		/// @brief Initialize service.
		void init() override {

			udjat_module_init();

			SystemService::init();

			if(Module::find("httpd")) {

				if(Module::find("information")) {
					cout << "http://localhost:8989/api/1.0/info/modules.xml" << endl;
					cout << "http://localhost:8989/api/1.0/info/workers.xml" << endl;
					cout << "http://localhost:8989/api/1.0/info/factories.xml" << endl;
					cout << "http://localhost:8989/api/1.0/info/services.xml" << endl;
				}

				cout << "http://localhost:8989/api/1.0/users.xml" << endl;
				cout << "http://localhost:8989/api/1.0/agent.xml" << endl;
				cout << "http://localhost:8989/api/1.0/alerts.xml" << endl;

				auto root = Abstract::Agent::root();
				if(root) {
					for(auto agent : *root) {
						cout << "http://localhost:8989/api/1.0/agent/" << agent->name() << ".html" << endl;
						cout << "http://localhost:8989/api/1.0/report/agent/" << agent->name() << ".html" << endl;
					}
				}

			}

		}

		/// @brief Deinitialize service.
		void deinit() override {
			cout << Application::Name() << "\t**** Deinitializing" << endl;
			Udjat::Module::unload();
		}

	public:
		Service() : SystemService{"./test.xml"} {
		}


	};

	Service().run(argc,argv);

	cout << "*** Test program finished" << endl;

	return 0;

}
