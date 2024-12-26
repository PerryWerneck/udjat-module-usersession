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

 /*
 #include <udjat/tools/user/session.h>
 #include <udjat/agent/abstract.h>
 #include <udjat/tools/value.h>
 #include <list>
*/
 
 namespace Udjat {

	namespace User {

		class UDJAT_API Agent : public Udjat::Abstract::Agent {
		private:

			struct Proxy {

				User::Event events = User::no_event;

				enum Filter : uint16_t {
					System_session 	= 0x0001,	///< @brief Activate on system sessions?
					User_Session	= 0x0002,	///< @brief Activate on user sessions?
					Remote 			= 0x0004,	///< @brief Activate on remote sessions?
					Locked 			= 0x0008,	///< @brief Activate on locked session?
					Unlocked		= 0x0010,	///< @brief Activate on unlocked session?
					Background		= 0x0020,	///< @brief Activate on background session?
					Foreground		= 0x0040,	///< @brief Activate on foreground session?
					Active			= 0x0080,	///< @brief Activate on active session?
					Inactive		= 0x0100,	///< @brief Activate on inactive session?
					All				= 0xFFFF
				} filter = All;

				std::shared_ptr<Activatable> activatable;	///< @brief The activatable for this event.

				time_t idle = 14400;			///< @brief Session idle time to emit 'pulse' events.
				time_t last_activation = 0;		///< @brief Time stamp of last activation.

				Proxy(const User::Event ev, const Filter f, std::shared_ptr<Activatable> a)
					: events{ev},filter{f},activatable{a} {						
				}

				void activate(const User::Session, const Abstract::Object &agent) const noexcept;

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

			Value & getProperties(Value &value) const override;
			bool getProperties(const char *path, Value &value) const override;

			/// @brief Get Agent value (seconds since last alert);
			time_t get() const noexcept;

		};

	}

 }
