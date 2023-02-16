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
 #include <udjat/tools/container.h>
 #include <udjat/tools/logger.h>
 #include <system_error>
 #include <udjat/agent.h>
 #include <udjat/factory.h>
 #include <udjat/alert/abstract.h>
 #include <udjat/agent/abstract.h>
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
	class UDJAT_PRIVATE Controller : public Udjat::User::Controller, private Udjat::Service {
	private:

		/// @brief List of active userlist agents.
		Container<UserList::Agent> agents;

	protected:

		std::shared_ptr<Udjat::User::Session> SessionFactory() noexcept override;

	public:
		Controller();

		static Controller & getInstance() {
			static Controller instance;
			return instance;
		}

		void start() override;
		void stop() override;

		inline void push_back(UserList::Agent *agent) {
			agents.push_back(agent);
		}

		inline void remove(UserList::Agent *agent) {
			agents.remove(agent);
		}

		bool for_each(std::function<bool(const UserList::Agent &agent)> callback) {
			return agents.for_each(callback);
		}

	};

	/// @brief Userlist agent.
	class UDJAT_PRIVATE Agent : public Udjat::Abstract::Agent {
	private:
		std::list<AlertProxy> proxies;
		void emit(Abstract::Alert &alert, Session &session) const noexcept;

		struct {
			unsigned int max_pulse_check = 600;	///< @brief Max value for pulse checks.
		} timers;

	public:
		Agent(const Agent&) = delete;
		Agent& operator=(const Agent &) = delete;
		Agent(Agent &&) = delete;
		Agent & operator=(Agent &&) = delete;

		Agent(const pugi::xml_node &node);
		virtual ~Agent();

		bool refresh() override;

		/// @brief Process event.
		/// @return true if an alert was activated.
		bool onEvent(Session &session, const Udjat::User::Event event) noexcept;

		bool push_back(const pugi::xml_node &node, std::shared_ptr<Activatable> activatable) override;

		Value & get(Value &value) const override;

		Value & getProperties(Value &value) const noexcept override;
		bool getProperties(const char *path, Value &value) const override;
		bool getProperties(const char *path, Report &report) const override;

		// void get(const Request &request, Report &report) override;
		// void get(const Request &request, Response &response) override;


	};

	/// @brief Proxy for user's alerts.
	class UDJAT_PRIVATE AlertProxy  {
	private:

		Udjat::User::Event event = (Udjat::User::Event) 0;

		struct {
			time_t timer = 0;				///< @brief Emission timer (for pulse alerts).
			bool system = false;			///< @brief Emit alert for system sessions?
			bool remote = false;			///< @brief Emit alert for remote sessions?
			bool locked = false;			///< @brief Emit alert on locked session?
			bool unlocked = true;			///< @brief Emit alert on unlocked session?
			bool background = false;		///< @brief Emit alert for background session?
			bool foreground = true;			///< @brief Emit alert for foreground session?
			bool active = true;				///< @brief Emit alert on active session?
			bool inactive = true;			///< @brief Emit alert on inactive session.

#ifndef _WIN32
			const char *classname = "";		///< @brief Filter by session classname.
			const char *service = "";		///< @brief Filter by session service.
#endif // !_WIN32

		} emit;

	protected:
		std::shared_ptr<Activatable> alert;

	public:

		AlertProxy(const pugi::xml_node &node, std::shared_ptr<Activatable> activatable);

		time_t timer() const noexcept {
			return emit.timer;
		}

		// Emit alert.
		void activate(const Agent &agent, const Session &session);

		/*
		inline std::shared_ptr<Udjat::Alert::Activation> ActivationFactory() const {
			return alert->ActivationFactory();
		}
		*/

		bool test(const Udjat::User::Session &session) const noexcept;

		inline bool test(Udjat::User::Event event) const noexcept {

#ifdef DEBUG
		Logger::trace() << " ***** alert-event=" << hex << ((unsigned int) this->event) << " event=" << ((unsigned int) event) << " rc=" << ((unsigned int) (this->event & event)) << dec << endl;
#endif // DEBUG

			return (this->event & event) != 0;
		}

	};

 }

