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
  * @brief Declares the user list.
  */

 #pragma once
 #include <config.h>
 #include <udjat/defs.h>

 #ifdef HAVE_DBUS
	#include <udjat/tools/dbus/connection.h>
 #endif // HAVE_DBUS

 namespace Udjat {

	namespace User {

		class Agent;

		/// @brief Singleton with the user's list.
		class UDJAT_API List {
		private:

			std::recursive_mutex guard;

			/// @brief Session list.
			std::list<Session *> sessions;

			/// @brief Agent list.
			std::list<Agent *> agents;

			/// @brief Initialize controller.
			void init() noexcept;

			/// @brief Deinitialize controller.
			void deinit() noexcept;

			/// @brief Initialize session.
			void init(Session &session);

			/// @brief Deinitialize session.
			void deinit(Session &session);

#ifdef _WIN32

			HWND hwnd = 0;
			static LRESULT WINAPI hwndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
			void load(bool starting) noexcept;

			// @brief			Find session by SID.
			// @param sid 		SID of the requested session.
			// @param create	If true will create a new session when not found.
			Session & find(DWORD sid);

#else

			Session & find(const char * sid);
			std::thread *monitor = nullptr;

			bool enabled = false;

			/// @brief Event fd
			int efd = -1;

			void wakeup();

			///< @brief Cached connection with the system bus listening for events.
			Udjat::DBus::SystemBus systembus;

#endif // _WIN32

			/// @brief System is going to sleep.
			void sleep();

			/// @brief System is resuming from sleep.
			void resume();

			/// @brief System is shutting down.
			void shutdown();

			/// @brief Update session list from system.
			//void refresh() noexcept;

			List();

		public:

			List(Controller &) = delete;
			List(Controller *) = delete;

			static List & getInstance();

			virtual ~List();

			/// @brief Start monitor, load sessions.
			void activate();

			/// @brief Stop monitor, unload sessions.
			void deactivate();

			bool for_each(const std::function<bool(User::Session &session)> &callback);

			bool for_each(const std::function<bool(User::Agent &agent)> &callback);

			inline size_t size() const {
				return sessions.size();
			}

			inline auto begin() {
				return sessions.begin();
			}

			inline auto end() {
				return sessions.end();
			}

			void push_back(User::Agent *agent);
			void remove(User::Agent *agent);

			void push_back(User::Session *session);
			void remove(User::Session *session);
		};

	}

 }
