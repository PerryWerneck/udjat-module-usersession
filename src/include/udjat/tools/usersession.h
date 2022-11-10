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
 #include <udjat/tools/object.h>
 #include <ostream>

 namespace Udjat {

	namespace User {

		class Session;

		/// @brief User events.
		enum Event : uint16_t {
			already_active	= 0x0001,		///< @brief Session is active on controller startup.
			still_active	= 0x0002,		///< @brief Session is active on controller shutdown.
			logon			= 0x0004,		///< @brief User logon detected.
			logoff			= 0x0008,		///< @brief User logoff detected.
			lock			= 0x0010,		///< @brief Session was locked.
			unlock			= 0x0020,		///< @brief Session was unlocked.
			foreground		= 0x0040,		///< @brief Session is in foreground.
			background		= 0x0080,		///< @brief Session is in background.
			sleep			= 0x0100,		///< @brief System is preparing to sleep.
			resume			= 0x0200,		///< @brief System is resuming from sleep.
			shutdown		= 0x0400,		///< @brief System is shutting down.

			pulse			= 0x0800,		///< @brief 'Pulse' event.
		};

		/// @brief Create an event id.
		/// @param names The list of event names delimited by ','
		/// @return The event id mixing all names.
		UDJAT_API Event EventFactory(const char *names);

		/// @brief Create an event id.
		/// @param names The list of event names delimited by ','
		/// @return The event id mixing all names.
		UDJAT_API Event EventFactory(const pugi::xml_node &node);

		/// @brief Session state, as reported by logind.
		/// @see sd_session_get_state
		enum State : uint8_t {
			SessionInBackground,		///< @brief Session logged in, but session not active, i.e. not in the foreground
			SessionInForeground,		///< @brief Session logged in and active, i.e. in the foreground
			SessionIsOpening,			///< @brief
			SessionIsClosing,			///< @brief Session nominally logged out, but some processes belonging to it are still around.

			SessionInUnknownState,		///< @brief Session in unknown state.
		};

		UDJAT_API State StateFactory(const char *statename);

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

			/// @brief Initialize session.
			void init(std::shared_ptr<Session> session);

			/// @brief Deinitialize session.
			void deinit(std::shared_ptr<Session> session);

#ifdef _WIN32
			HWND hwnd = 0;
			static LRESULT WINAPI hwndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
			void load(bool starting) noexcept;

			// @brief			Find session by SID.
			// @param sid 		SID of the requested session.
			// @param create	If true will create a new session when not found.
			std::shared_ptr<Session> find(DWORD sid, bool create = true);
#else
			std::shared_ptr<Session> find(const char * sid);
			std::thread *monitor = nullptr;
			bool enabled = false;
#endif // _WIN32

			/// @brief System is going to sleep.
			void sleep();

			/// @brief System is resuming from sleep.
			void resume();

			/// @brief System is shutting down.
			void shutdown();

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

			/// @brief Start monitor, load sessions.
			void activate();

			/// @brief Stop monitor, unload sessions.
			void deactivate();

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
		class UDJAT_API Session : public Udjat::Abstract::Object {
		private:
			friend class Controller;

			struct {
				State state = User::SessionInUnknownState;	///< @brief Current user state.
				bool alive = false;							///< @brief True if the session is alive.
				bool locked = false;						///< @brief True if the session is locked.
#ifdef _WIN32
				bool remote = false;						///< @brief True if the session is remote.
				bool system = true;							///< @brief True if its a system session.
#else
				uint8_t remote = 0xFF;						///< @brief Remote state.
#endif // _WIN32
			} flags;

			std::string username;		///< @brief Windows user name.

#ifdef _WIN32

			DWORD sid = 0;					///< @brief Windows Session ID.
#else

			std::string sid;				///< @brief LoginD session ID.
			std::string dbpath;				///< @brief D-Bus session path.
			uid_t uid = -1;					///< @brief Session user id.
			void *bus = nullptr;			///< @brief Connection with the user's bus
			const char * cname = nullptr;	///< @brief Session class.

#endif // _WIN32

		protected:
			/// @brief Emit event, update timers.
			void emit(const Event &event) noexcept;

			/// @brief Notify event emission (dont call it directly).
			virtual Session & onEvent(const Event &event) noexcept;

		public:
			Session();
			virtual ~Session();

			/// @brief Get session name or id.
			std::string to_string() const noexcept override;

			const char * name() const noexcept override;

			const char * name(bool update) const noexcept;

			bool getProperty(const char *key, std::string &value) const noexcept override;

			/// @brief Is this session a remote one?
			bool remote() const;

			/// @brief Is this session locked?
			bool locked() const;

			/// @brief Is this session active?
			bool active() const noexcept;

			/// @brief Get session state.
			inline State state() const noexcept {
				return flags.state;
			}

			/// @brief Is this a 'system' session?
			bool system() const;

			/// @brief Is this a 'foreground' session?
			bool foreground() const noexcept {
				return flags.state == SessionInForeground;
			}

			Session & set(const User::State state);

			/// @brief Is this session alive?
			inline bool alive() const noexcept {
				return flags.alive;
			}

#ifndef _WIN32
			/// @brief Get session's user id
			int userid() const;

			/// @brief Get X11 display of the session.
			std::string display() const;

			/// @brief Get session type.
			std::string type() const;

			/// @brief The name of the service (as passed during PAM session setup) that registered the session.
			std::string service() const;

			/// @brief The class of the session.
			const char * classname() const noexcept;

			/// @brief The D-Bus session path.
			std::string path() const;

			/// @brief Get environment value from user session.
			std::string getenv(const char *varname) const;

			/// @brief Execute function as user's effective id.
			static void call(const uid_t uid, const std::function<void()> exec);

			/// @brief Execute function as user's effective id.
			void call(const std::function<void()> exec);
#endif // _WIN32

		};
	}

 }

 namespace std {

	UDJAT_API const std::string to_string(const Udjat::User::Event event, bool description = false) noexcept;

	inline ostream& operator<< (ostream& os, const Udjat::User::Event event) {
		return os << to_string(event, true);
	}

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

	UDJAT_API const char * to_string(const Udjat::User::State state) noexcept;

	inline ostream& operator<< (std::ostream& os, const Udjat::User::State state) {
		return os << to_string(state);
	}

 }

