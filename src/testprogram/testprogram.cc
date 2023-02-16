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
 #include <udjat/tools/logger.h>

 using namespace std;
 using namespace Udjat;

//---[ Implement ]------------------------------------------------------------------------------------------

int main(int argc, char **argv) {

	/*
	class Service : public SystemService {
	protected:
		/// @brief Initialize service.
		void init() override {

			udjat_module_init();

			SystemService::init();

			if(Module::find("httpd")) {

				debug("http://localhost:8989");

				if(Module::find("information")) {
					debug("http://localhost:8989/api/1.0/info/modules.xml");
					debug("http://localhost:8989/api/1.0/info/workers.xml");
					debug("http://localhost:8989/api/1.0/info/factories.xml");
					debug("http://localhost:8989/api/1.0/info/services.xml");
				}

				debug("http://localhost:8989/api/1.0/users.xml");
				debug("http://localhost:8989/api/1.0/agent.xml");
				debug("http://localhost:8989/api/1.0/alerts.xml");

				auto root = Abstract::Agent::root();
				if(root) {
					for(auto agent : *root) {
						debug("http://localhost:8989/api/1.0/agent/",agent->name(),".html");
						debug("http://localhost:8989/api/1.0/report/agent/",agent->name(),".html");
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

	Logger::verbosity(9);

	Service().run(argc,argv);

	cout << "*** Test program finished" << endl;
	*/

	Logger::verbosity(9);
	Logger::redirect();

	udjat_module_init();

	Application{}.run(argc,argv,"./test.xml");

	return 0;

}
