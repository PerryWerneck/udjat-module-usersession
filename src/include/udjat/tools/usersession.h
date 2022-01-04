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

 #include <udjat/defs.h>
 #include <memory>
 #include <string>
 #include <mutex>
 #include <list>
 #include <thread>

 namespace Udjat {

	namespace User {

		class Session;

		/// @brief User events.
		enum Event : uint8_t {
			logon,
			logoff
		};

		/// @brief User session controller.
		class UDJAT_API Controller {
		private:

			std::mutex guard;

			/// @brief Session list.
			std::list<std::shared_ptr<Session>> sessions;

			/// @brief Update session list from system.
			void refresh() noexcept;

#ifdef _WIN32
			std::shared_ptr<Session> find(DWORD sid);
#else
			std::shared_ptr<Session> find(const char * sid);

			std::thread *monitor = nullptr;
			bool enabled = false;

#endif // _WIN32

		protected:

#ifndef _WIN32
			void start();
#endif // !_WIN32

			/// @brief Session factory called every time the controller detects a new user session.
			virtual std::shared_ptr<Session> SessionFactory() noexcept;

		public:
			Controller();
			virtual ~Controller();

		};

		/// @brief User session.
		class UDJAT_API Session {
		private:
			friend class Controller;

			struct {
				bool alive = false;		///< @brief True if the session is alive.
				bool remote = false;	///< @brief True if it is a remote session.
				bool locked = false;	///< @brief True if the session is locked.
			} state;

#ifdef _WIN32

			DWORD sid = 0;				///< @brief Windows Session ID.

#else

			std::string sid;			///< @brief LoginD session ID.

#endif // _WIN32

		protected:
			virtual Session & onEvent(const Event &event) noexcept;

		public:
			Session();
			virtual ~Session();

			/// @brief Get session name or id.
			std::string to_string() const noexcept;

			/// @brief Is this session a remote one?
			inline bool remote() const noexcept {
				return state.remote;
			}

			/// @brief Is this session locked?
			inline bool locked() const noexcept {
				return state.locked;
			}

			/// @brief Is this session alive?
			inline bool alive() const noexcept {
				return state.alive;
			}

		};
	}

 }

