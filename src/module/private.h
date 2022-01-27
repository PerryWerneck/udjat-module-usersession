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
 #include <udjat/tools/usersession.h>
 #include <udjat/tools/mainloop.h>
 #include <udjat/alert.h>
 #include <system_error>
 #include <udjat/agent.h>
 #include <udjat/factory.h>
 #include <list>
 #include <memory>

 class Controller;

 using namespace std;
 using namespace Udjat;

 namespace UserList {

	class Agent;
	class Alert;

	/// @brief Module info
	extern const Udjat::ModuleInfo info;

	/// @brief Singleton with the real userlist.
	class Controller : public Udjat::User::Controller, private Udjat::MainLoop::Service {
	private:
		static std::mutex guard;

		/// @brief List of active userlist agents.
		std::list<Agent *> agents;

	protected:

		std::shared_ptr<Udjat::User::Session> SessionFactory() noexcept override;

		public:
		Controller();
		static std::shared_ptr<Controller> getInstance();

		void start() override;
		void stop() override;

		void insert(UserList::Agent *agent);
		void remove(UserList::Agent *agent);

		void for_each(std::function<void(UserList::Agent &agent)> callback);

	};

	/// @brief Userlist agent.
	class Agent : public Udjat::Abstract::Agent {
	private:
		std::shared_ptr<UserList::Controller> controller;
		std::list<shared_ptr<UserList::Alert>> alerts;

	public:

		class Factory : public Udjat::Factory {
		public:
			Factory();
			bool parse(Udjat::Abstract::Agent &parent, const pugi::xml_node &node) const override;
		};

		Agent(const pugi::xml_node &node);
		virtual ~Agent();

		inline void insert(shared_ptr<UserList::Alert> alert) {
			alerts.push_back(alert);
		}

		void onEvent(Udjat::User::Session &session, const Udjat::User::Event event) noexcept;

		void append_alert(const pugi::xml_node &node) override;

	};

	/// @brief Userlist alert.
	class Alert : public Udjat::Alert {
	private:
		Udjat::User::Event event = (Udjat::User::Event) -1;

		/// @brief Emit alert for system sessions?
		bool system = false;

		/// @brief Emit alert for remote sessions?
		bool remote = false;

		std::shared_ptr<Abstract::Alert::Activation> ActivationFactory(const std::function<void(std::string &str)> &expander) const override;

	public:

		class Factory : public Udjat::Factory {
		public:
			Factory();
			bool parse(Udjat::Abstract::Agent &parent, const pugi::xml_node &node) const override;
		};

		const char * name() const noexcept {
			return Abstract::Alert::name;
		}

		Alert(const pugi::xml_node &node);
		virtual ~Alert();

		static void onEvent(shared_ptr<UserList::Alert> alert, const Udjat::User::Session &session, const Udjat::User::Event event) noexcept;

	};

 }

