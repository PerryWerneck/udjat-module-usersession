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
  * @brief Declare alert for user events.
  */

/*
 #pragma once
 #include <udjat/defs.h>
 #include <udjat/tools/xml.h>
 #include <udjat/tools/user/session.h>

 namespace Udjat {

	namespace User {

		/// @brief Alert on user events.
		class UDJAT_API Alert {
		private:

			Udjat::User::Event event = Udjat::User::no_event;

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

			Alert(const XML::Node &node, std::shared_ptr<Activatable> activatable);

			time_t timer() const noexcept {
				return emit.timer;
			}

			// Emit alert.
			void activate(const Agent &agent, const Session &session);

			bool test(const Udjat::User::Session &session) const noexcept;

			inline bool test(Udjat::User::Event event) const noexcept {
				return (this->event & event) != 0;
			}

		};

	}

 }
*/
