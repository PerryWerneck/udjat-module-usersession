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
 #include <udjat/tools/user/session.h>
 #include <udjat/agent/abstract.h>
 #include <udjat/request.h>
 #include <udjat/tools/value.h>
 #include <list>

 namespace Udjat {

	namespace User {

		class Alert;

		class UDJAT_API Agent : public Udjat::Abstract::Agent {
		private:

			std::list<Alert> proxies;

			/// @brief Timestamp of the last alert emission.
			time_t alert_timestamp = time(0);

			void emit(Abstract::Alert &alert, Session &session) const noexcept;

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

			bool push_back(const pugi::xml_node &node, std::shared_ptr<Activatable> activatable) override;

			Value & get(Value &value) const override;

			Value & getProperties(Value &value) const override;
			bool getProperties(const char *path, Value &value) const override;
			bool getProperties(const char *path, Report &report) const override;

			/// @brief Get Agent value (seconds since last alert);
			time_t get() const noexcept;

		};

	}

 }
