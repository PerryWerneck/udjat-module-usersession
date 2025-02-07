/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2024 Perry Werneck <perry.werneck@gmail.com>
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

 /**
  * @brief Declare user agent.
  */

 #pragma once
 #include <udjat/defs.h>
 #include <udjat/tools/xml.h>
 #include <udjat/agent/abstract.h>
 #include <udjat/tools/activatable.h>
 #include <udjat/tools/user/session.h>
				
 namespace Udjat {

	namespace User {

		class UDJAT_API Agent : public Udjat::Abstract::Agent {
		private:

			/// @brief Proxy for activatables parsing user filters.
			struct Proxy {

				User::Event events = User::no_event;
				Session::Type filter = Session::All;

#ifndef _WIN32
				const char *classname = nullptr;
				const char *servicename = nullptr;
#endif // !_WIN32

				std::shared_ptr<Activatable> activatable;	///< @brief The activatable for this event.

				time_t timer = 0;	///< @brief Session idle time to emit 'pulse' events (if enabled).

				Proxy(const XML::Node &node, const User::Event event, std::shared_ptr<Activatable> activatable);

				void activate(const User::Session &session, const Abstract::Object &agent) const noexcept;

			};

			std::list<Proxy> proxies;

			/// @brief Timestamp of the last alert emission.
			time_t alert_timestamp = time(0);

			void emit(Udjat::Activatable &activatable, Session &session) const noexcept;

			struct {
				unsigned int max_pulse_check = 600;	///< @brief Max value for pulse checks.
			} timers;

		public:
			Agent(const Agent&) = delete;
			Agent& operator=(const Agent &) = delete;
			Agent(Agent &&) = delete;
			Agent & operator=(Agent &&) = delete;

			Agent(const XML::Node &node);
			virtual ~Agent();

			bool refresh() override;

			/// @brief Process event.
			/// @return true if an alert was activated.
			bool onEvent(Session &session, const Udjat::User::Event event) noexcept;

			/// @brief Check XML for special user events, add it on proxy list if necessary.
			/// @param node The activatable description.
			/// @param activatable The activatable built for this event.
			/// @return true if the event was pushed.
			bool push_back(const pugi::xml_node &node, std::shared_ptr<Activatable> activatable) override;

			Value & get(Value &value) const override;

			bool getProperties(const char *path, Value &value) const override;

			/// @brief Get Agent value (seconds since last alert);
			time_t get() const noexcept;

		};

	}

 }
