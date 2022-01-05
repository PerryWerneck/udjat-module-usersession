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

 #include <winsock2.h>
 #include <windows.h>
 #include <wtsapi32.h>
 #include <udjat/win32/exception.h>

 #include <udjat/tools/usersession.h>
 #include <udjat/win32/exception.h>
 #include <cstring>
 #include <iostream>

 using namespace std;

 #ifndef PACKAGE_NAME
	#define PACKAGE_NAME "UDJAT-USER-MONITOR"
 #endif // PACKAGE_NAME

 #define WM_START		WM_USER+100
 #define WM_REFRESH		WM_USER+101

 namespace Udjat {

	void User::Controller::refresh() noexcept {
		PostMessage(hwnd,WM_REFRESH,0,0);
	}

	/// @brief Find session (Requires an active guard!!!)
	std::shared_ptr<User::Session> User::Controller::find(const DWORD sid) {

		for(auto session : sessions) {
			if(session->sid == sid) {
				return session;
			}
		}

		// Not found, create a new one.
		std::shared_ptr<Session> session = SessionFactory();
		session->sid = sid;
		sessions.push_back(session);

		return session;
	}

	User::Controller::Controller() {

		//
		// Register a new object window class
		//
		static ATOM	winClass = 0;
		if(!winClass) {

			WNDCLASSEX wc;

			memset(&wc,0,sizeof(wc));

			wc.cbSize 			= sizeof(wc);
			wc.style  			= CS_HREDRAW | CS_VREDRAW;
			wc.lpfnWndProc  	= Controller::hwndProc;
			wc.hInstance  		= GetModuleHandle(NULL);
			wc.lpszClassName  	= PACKAGE_NAME;
			wc.cbWndExtra		= sizeof(LONG_PTR);

			winClass = RegisterClassEx(&wc);

			if(!winClass) {
				throw Win32::Exception("RegisterClassEx has failed");
			}

		}

	}

	User::Controller::~Controller() {

		lock_guard<mutex> lock(guard);
		if(hwnd) {
			DestroyWindow(hwnd);
			hwnd = 0;
		}

	}

	void User::Controller::start() {

		lock_guard<mutex> lock(guard);

		if(hwnd) {
			throw runtime_error("User Session monitor is already started");
		}

		hwnd = CreateWindow(
			PACKAGE_NAME,
			"user-monitor",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			0,
			CW_USEDEFAULT,
			0,
			NULL,
			NULL,
			GetModuleHandle(NULL),
			NULL
		);

		if(!hwnd) {
			throw Win32::Exception("Cant create object window");
		}

		SetWindowLongPtr(hwnd, 0, (LONG_PTR) this);

		if(!WTSRegisterSessionNotification(hwnd,NOTIFY_FOR_ALL_SESSIONS)) {
			cerr << "users\t" << Win32::Exception::format("WTSRegisterSessionNotification") << endl;
		}

		PostMessage(hwnd,WM_START,0,0);

	}

	void User::Controller::stop() {

		lock_guard<mutex> lock(guard);
		if(!hwnd) {
			throw runtime_error("User Session monitor is already stopped");
		}
		DestroyWindow(hwnd);

	}

	void User::Controller::load(bool starting) noexcept {

		WTS_SESSION_INFO	* sessions;
		DWORD 				  count = 0;

		if(!WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE,0,1,&sessions,&count)) {
			cerr << "users\t" << Win32::Exception::format("WTSEnumerateSessions") << endl;
			return;
		}

		try {
			for(DWORD ix = 0; ix < count; ix++) {
				std::shared_ptr<Session> session;

				if(starting) {
					session = SessionFactory();
					session->sid = sessions[ix].SessionId;
					this->sessions.push_back(session);
				} else {
					session = find(sessions[ix].SessionId);
				}
				session->state.alive = true;

				// https://msdn.microsoft.com/en-us/library/aa383860(v=vs.85).aspx
				switch(sessions[ix].State) {
				case WTSActive:
					cout << "@" << session->sid << "\tA user is logged on to the WinStation." << endl;
					break;

				case WTSConnected:
					cout << "@" << session->sid << "\tThe WinStation is connected to the client." << endl;
					break;

				case WTSConnectQuery:
					cout << "@" << session->sid << "\tThe WinStation is in the process of connecting to the client." << endl;
					break;

				case WTSShadow:
					cout << "@" << session->sid << "\tThe WinStation is shadowing another WinStation." << endl;
					break;

				case WTSDisconnected:
					cout << "@" << session->sid << "\tThe WinStation is active but the client is disconnected." << endl;
					session->state.locked = true;
					if(!starting) {
						session->onEvent(lock);
					}
					break;

				case WTSIdle:
					cout << "@" << session->sid << "\tThe WinStation is waiting for a client to connect." << endl;
					break;

				case WTSListen:
					cout << "@" << session->sid << "\tThe WinStation is listening for a connection." << endl;
					break;

				case WTSReset:
					cout << "@" << session->sid << "\tThe WinStation is being reset." << endl;
					break;

				case WTSDown:
					cout << "@" << session->sid << "\tThe WinStation is down due to an error." << endl;
					break;

				case WTSInit:
					cout << "@" << sessions[ix].SessionId << "\tThe WinStation is initializing." << endl;
					break;

				default:
					cout << "@" << sessions[ix].SessionId << "\tUnexpected session state" << endl;
					break;
				}

			}

		} catch(const exception &e) {

			cerr << "users\t" << e.what() << endl;

		} catch(...) {

			cerr << "users\tUnexpected error loading user sessions" << endl;

		}

		WTSFreeMemory(sessions);

	}

	LRESULT WINAPI User::Controller::hwndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

		User::Controller & controller = *((User::Controller *) GetWindowLongPtr(hWnd,0));

		// https://wiki.winehq.org/List_Of_Windows_Messages
		switch(uMsg) {
		case WM_WTSSESSION_CHANGE:

			try {

				auto session = controller.find((DWORD) lParam);

				switch((int) wParam) {
				case WTS_SESSION_LOCK:				// The session has been locked.
					cout << "users\tWTS_SESSION_LOCK " << session->sid << endl;
					if(!session->state.locked) {
						session->state.locked = true;
						session->onEvent(lock);
					}
					break;

				case WTS_SESSION_UNLOCK:			// The session identified has been unlocked.
					cout << "users\tWTS_SESSION_UNLOCK " << session->sid << endl;
					if(session->state.locked) {
						session->state.locked = false;
						session->onEvent(unlock);
					}
					break;

				case WTS_CONSOLE_CONNECT:			// The session was connected to the console terminal or RemoteFX session.
					cout << "users\tWTS_CONSOLE_CONNECT " << session->sid << endl;
					break;

				case WTS_CONSOLE_DISCONNECT:		// The session was disconnected from the console terminal or RemoteFX session.
					cout << "users\tWTS_CONSOLE_DISCONNECT " << session->sid << endl;
					break;

				case WTS_REMOTE_CONNECT:			// The session was connected to the remote terminal.
					cout << "users\tWTS_REMOTE_CONNECT " << session->sid << endl;
					session->state.remote = true;
					break;

				case WTS_REMOTE_DISCONNECT:			// The session was disconnected from the remote terminal.
					cout << "users\tWTS_REMOTE_DISCONNECT " << session->sid << endl;
					session->state.remote = true;
					break;

				case WTS_SESSION_LOGON:				// A user has logged on to the session.
					cout << "users\tWTS_SESSION_LOGON " << session->sid << endl;
					session->onEvent(logon);
					break;

				case WTS_SESSION_LOGOFF:			// A user has logged off the session.
					cout << "users\tWTS_SESSION_LOGOFF " << session->sid << endl;
					session->onEvent(logoff);
					session->state.alive = false;
					{
						lock_guard<mutex> lock(controller.guard);
						controller.sessions.remove(session);
					}
					break;

				case WTS_SESSION_REMOTE_CONTROL:	// The session has changed its remote controlled status.
					cout << "users\tWTS_SESSION_REMOTE_CONTROL " << session->sid << endl;
					break;

				case WTS_SESSION_CREATE:			// Reserved for future use.
					cout << "users\tWTS_SESSION_CREATE " << session->sid << endl;
					break;

				case WTS_SESSION_TERMINATE:			// Reserved for future use.
					cout << "users\tWTS_SESSION_TERMINATE " << session->sid << endl;
					break;

				default:
					cerr	<< "users\tWM_WTSSESSION_CHANGE sent an unexpected state '"
							<< ((int) wParam) << "'" << endl;
				}

			} catch(const std::exception &e) {

				cerr << "users\tError updating SID " << ((DWORD) lParam) << ": " << e.what() << endl;

			} catch(...) {

				cerr << "users\tUnexpected error updating SID " << ((DWORD) lParam) << endl;

			}
			break;

		case WM_CLOSE:
			cout << "users\tUser monitor window is closing (WM_CLOSE)" << endl;
			break;

		case WM_DESTROY:
			cout << "users\tUser monitor window was destroyed (WM_DESTROY)" << endl;
			if(controller.hwnd) {
				controller.deinit();
				WTSUnRegisterSessionNotification(hWnd);
				controller.hwnd = 0;
			}
			break;

		case WM_ENDSESSION:
			cout << "users\tWM_ENDSESSION" << endl;
			return DefWindowProc(hWnd, uMsg, wParam, lParam);

		case WM_START:
			cout << "users\tLoading active sessions" << endl;
			controller.load(true);
			controller.init();
			break;

		case WM_REFRESH:
			controller.load(false);
			break;

		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);

		}

		return 0;

	}

 }
