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

	/// @brief User Session Agent
	class Session : public Udjat::User::Session {
	private:

		/// @brief Timestamp of the last alert emission.
		time_t lastreset = 0;

	protected:
		Udjat::User::Session & onEvent(const Udjat::User::Event &event) noexcept override;

	public:
		Session() = default;

		/// @brief Get time of the last reset.
		inline time_t alerttime() const noexcept {
			return lastreset;
		}

		/// @brief Reset timer.
		inline void reset() noexcept {
			lastreset = time(0);
		}

	};

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
		std::list<shared_ptr<Abstract::Alert>> alerts;

		 void emit(Abstract::Alert &alert, Session &session) const noexcept;

	public:
		Agent(const pugi::xml_node &node);
		virtual ~Agent();

		bool refresh() override;

		void push_back(std::shared_ptr<Udjat::Abstract::Alert> alert) override;

		/// @brief Process event.
		/// @return true if an alert was activated.
		bool onEvent(Session &session, const Udjat::User::Event event) noexcept;

	};

	/// @brief Userlist alert.
	class Alert : public Udjat::Alert {
	private:
		friend class Agent;

		Udjat::User::Event event = (Udjat::User::Event) -1;

		struct {
			time_t timer = 0;			///< @brief Emission timer (for pulse alerts).
			bool system = false;		///< @brief Emit alert for system sessions?
			bool remote = false;		///< @brief Emit alert for remote sessions?
			bool locked = false;		///< @brief Emit alert on locked session?
			bool unlocked = true;		///< @brief Emit alert on unlocked session?
			bool background = false;	///< @brief Emit alert for background session?
			bool foreground = true;		///< @brief Emit alert for foreground session?
		} emit;

		std::shared_ptr<Abstract::Alert::Activation> ActivationFactory() const override;

	public:

		Alert(const pugi::xml_node &node);
		virtual ~Alert();

		// static bool onEvent(shared_ptr<Abstract::Alert> alert, const Udjat::User::Session &session, const Udjat::User::Event event) noexcept;

		time_t timer() const noexcept {
			return emit.timer;
		}

		bool test(const Udjat::User::Session &session) const noexcept;

		inline bool test(Udjat::User::Event event) const noexcept {
			return this->event == event;
		}

	};

 }

