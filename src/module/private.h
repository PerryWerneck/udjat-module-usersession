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
	class AlertProxy;

	/// @brief User Session Agent
	class UDJAT_PRIVATE Session : public Udjat::User::Session {
	private:

		/// @brief Timestamp of the last alert emission.
		time_t lastreset = time(0);

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
	class UDJAT_PRIVATE Controller : public Udjat::User::Controller, private Udjat::MainLoop::Service {
	private:
		static std::mutex guard;

		/// @brief List of active userlist agents.
		std::list<Agent *> agents;

	protected:

		std::shared_ptr<Udjat::User::Session> SessionFactory() noexcept override;

	public:
		Controller();
		static Controller & getInstance();

		void start() override;
		void stop() override;

		void insert(UserList::Agent *agent);
		void remove(UserList::Agent *agent);

		void for_each(std::function<void(UserList::Agent &agent)> callback);

	};

	/// @brief Userlist agent.
	class UDJAT_PRIVATE Agent : public Udjat::Abstract::Agent {
	private:
		std::list<AlertProxy> alerts;
		void emit(Abstract::Alert &alert, Session &session) const noexcept;

	public:
		Agent(const pugi::xml_node &node);
		virtual ~Agent();

		bool refresh() override;

		/// @brief Process event.
		/// @return true if an alert was activated.
		bool onEvent(Session &session, const Udjat::User::Event event) noexcept;

		void push_back(const pugi::xml_node &node, std::shared_ptr<Abstract::Alert> alert) override;

		void get(const Request &request, Report &report) override;

	};

	/// @brief Proxy for user's alerts.
	class UDJAT_PRIVATE AlertProxy  {
	private:

		Udjat::User::Event event = (Udjat::User::Event) 0;

		struct {
			time_t timer = 0;			///< @brief Emission timer (for pulse alerts).
			bool system = false;		///< @brief Emit alert for system sessions?
			bool remote = false;		///< @brief Emit alert for remote sessions?
			bool locked = false;		///< @brief Emit alert on locked session?
			bool unlocked = true;		///< @brief Emit alert on unlocked session?
			bool background = false;	///< @brief Emit alert for background session?
			bool foreground = true;		///< @brief Emit alert for foreground session?
		} emit;

	protected:
		std::shared_ptr<Abstract::Alert> alert;

	public:

		AlertProxy(const pugi::xml_node &node, std::shared_ptr<Abstract::Alert> a);

		time_t timer() const noexcept {
			return emit.timer;
		}

		inline std::shared_ptr<Udjat::Alert::Activation> ActivationFactory() const {
			return alert->ActivationFactory();
		}

		bool test(const Udjat::User::Session &session) const noexcept;

		inline bool test(Udjat::User::Event event) const noexcept {
#ifdef DEBUG
			cout << " ***** alert-event=" << hex << ((unsigned int) this->event) << " event=" << ((unsigned int) event) << " rc=" << ((unsigned int) (this->event & event)) << dec << endl;
#endif // DEBUG
			return (this->event & event) != 0;
		}

	};

 }

