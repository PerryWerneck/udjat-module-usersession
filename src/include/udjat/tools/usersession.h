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
 #include <functional>

#ifndef _WIN32
	#include <udjat/tools/dbus.h>
#endif // _WIN32

 namespace Udjat {

	namespace User {

		class Session;

		/// @brief User events.
		enum Event : uint8_t {
			already_active,		///< @brief Session is active on controller startup.
			still_active,		///< @brief Session is active on controller shutdown.
			logon,				///< @brief User logon detected.
			logoff,				///< @brief User logoff detected.
			lock,				///< @brief Session was locked.
			unlock,				///< @brief Session was unlocked.
		};

		/// @brief User session controller.
		class UDJAT_API Controller {
		private:

			std::mutex guard;

			/// @brief Session list.
			std::list<std::shared_ptr<Session>> sessions;

			/// @brief Initialize controller.
			void init() noexcept;

			/// @brief Deinitialize controller.
			void deinit() noexcept;

#ifdef _WIN32
			HWND hwnd = 0;
			static LRESULT WINAPI hwndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
			void load(bool starting) noexcept;
			std::shared_ptr<Session> find(DWORD sid);
#else
			std::shared_ptr<Session> find(const char * sid);
			std::thread *monitor = nullptr;
			bool enabled = false;
			void setup(Session *session);
#endif // _WIN32

		protected:

			/// @brief Session factory called every time the controller detects a new user session.
			virtual std::shared_ptr<Session> SessionFactory() noexcept;

			/// @brief Update session list from system.
			void refresh() noexcept;

		public:
			Controller(Controller &) = delete;
			Controller(Controller *) = delete;

			Controller();
			virtual ~Controller();

			/// @brief Start monitor, load users.
			void load();

			/// @brief Stop monitor, unload users.
			void unload();

			void for_each(std::function<void(std::shared_ptr<Session>)> callback);

			inline size_t size() const {
				return sessions.size();
			}

			inline std::list<std::shared_ptr<Session>>::iterator begin() {
				return sessions.begin();
			}

			inline std::list<std::shared_ptr<Session>>::iterator end() {
				return sessions.end();
			}

		};

		/// @brief User session.
		class UDJAT_API Session {
		private:
			friend class Controller;

			struct {
				bool alive = false;		///< @brief True if the session is alive.
#ifdef _WIN32
				bool remote = false;	///< @brief True if the session is remote.
				bool locked = false;	///< @brief True if the session is locked.
				bool active = false;	///< @brief True if the session is active.
#endif // _WIN32
			} state;

#ifdef _WIN32

			DWORD sid = 0;				///< @brief Windows Session ID.

#else

			std::string sid;						///< @brief LoginD session ID.
			DBus::Connection *bus = nullptr;		///< @brief Connection with the user's bus

#endif // _WIN32

		protected:
			virtual Session & onEvent(const Event &event) noexcept;

		public:
			Session();
			virtual ~Session();

			/// @brief Get session name or id.
			std::string to_string() const noexcept;

			/// @brief Is this session a remote one?
			bool remote() const;

			/// @brief Is this session locked?
			bool locked() const;

			/// @brief Is this session active?
			bool active() const;

			/// @brief Is this a 'system' session?
			bool system() const;

			/// @brief Is this session alive?
			inline bool alive() const noexcept {
				return state.alive;
			}

#ifndef _WIN32
			/// @brief Get session's user id
			int userid() const;

			/// @brief Get environment value from user session.
			std::string getenv(const char *varname) const;
#endif // _WIN32

		};
	}

 }

 namespace std {

	const char * to_string(const Udjat::User::Event event) noexcept;

	inline const string to_string(const Udjat::User::Session &session) noexcept {
		return session.to_string();
	}

	inline const string to_string(std::shared_ptr<Udjat::User::Session> session) noexcept {
		return session->to_string();
	}

	inline ostream& operator<< (ostream& os, const Udjat::User::Session &session) {
		return os << session.to_string();
	}

	inline ostream& operator<< (ostream& os, const std::shared_ptr<Udjat::User::Session> session) {
		return os << session->to_string();
	}

 }

