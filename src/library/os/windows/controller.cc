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

 #include <config.h>
 #include <winsock2.h>
 #include <windows.h>
 #include <wtsapi32.h>
 #include <udjat/win32/exception.h>

 #include <udjat/tools/user/session.h>
 #include <udjat/win32/exception.h>
 #include <udjat/tools/logger.h>
 #include <cstring>
 #include <iostream>

 using namespace std;

 #define WM_START		WM_USER+100
 #define WM_REFRESH		WM_USER+101

 namespace Udjat {

	void User::List::refresh() noexcept {
		PostMessage(hwnd,WM_REFRESH,0,0);
	}

	/// @brief Find session (Requires an active guard!!!)
	std::shared_ptr<User::Session> User::List::find(const DWORD sid, bool create) {

		for(auto session : sessions) {
			if(session->sid == sid) {
				return session;
			}
		}

		// Not found.

		if(create) {
			// Create a new session.
			std::shared_ptr<Session> session = SessionFactory();
			session->sid = sid;
			sessions.push_back(session);
			return session;
		}

		// Return an empty session.
		return std::shared_ptr<User::Session>();

	}

	User::List::Controller() {

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

	User::List::~Controller() {
		if(hwnd) {
			DestroyWindow(hwnd);
		}
	}

	void User::List::activate() {

		lock_guard<mutex> lock(guard);

		if(hwnd) {
			return;
		}

		cout << "users\tStarting user session monitor" << endl;
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

	void User::List::deactivate() {

		if(hwnd) {
			cout << "users\tStopping user session monitor" << endl;
			DestroyWindow(hwnd);
		}

	}

	void User::List::load(bool starting) noexcept {

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
				session->flags.alive = true;

				// https://msdn.microsoft.com/en-us/library/aa383860(v=vs.85).aspx
				switch(sessions[ix].State) {
				case WTSActive:
					session->name(true);
					session->trace() << "WTSActive(" << session->sid << "): A user is logged on to the WinStation." << endl;
					break;

				case WTSConnected:
					session->trace() << "WTSConnected(" << session->sid << "): The WinStation is connected to the client." << endl;
					break;

				case WTSConnectQuery:
					session->trace() << "WTSConnectQuery(" << session->sid << "): The WinStation is in the process of connecting to the client." << endl;
					break;

				case WTSShadow:
					session->trace() << "WTSShadow(" << session->sid << "): The WinStation is shadowing another WinStation." << endl;
					break;

				case WTSDisconnected:
					session->trace() << "WTSDisconnected(" << session->sid << "): The WinStation is active but the client is disconnected." << endl;
					session->flags.locked = true;
					if(!starting) {
						session->emit(lock);
					}
					break;

				case WTSIdle:
					session->trace() << "WTSIdle(" << session->sid << "): The WinStation is waiting for a client to connect." << endl;
					break;

				case WTSListen:
					session->trace() << "WTSListen(" << session->sid << "): The WinStation is listening for a connection." << endl;
					break;

				case WTSReset:
					session->trace() << "WTSReset(" << session->sid << "): The WinStation is being reset." << endl;
					break;

				case WTSDown:
					session->trace() << "WTSDown(" << session->sid << "): The WinStation is down due to an error." << endl;
					break;

				case WTSInit:
					session->trace() << "WTSInit(" << session->sid << "): The WinStation is initializing." << endl;
					break;

				default:
					session->trace() << sessions[ix].State << "(" << session->sid << "): Unexpected session state" << endl;
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

	LRESULT WINAPI User::List::hwndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

		User::Controller & controller = *((User::Controller *) GetWindowLongPtr(hWnd,0));

		debug("uMsg=",uMsg);

		// https://wiki.winehq.org/List_Of_Windows_Messages
		switch(uMsg) {
		case WM_POWERBROADCAST:
			try {

				switch(wParam) {
				case PBT_APMPOWERSTATUSCHANGE:
					cout << "users\tPower status has changed." << endl;
					break;

				case PBT_POWERSETTINGCHANGE:
					cout << "users\tA power setting change event has been received." << endl;
					break;

				case PBT_APMRESUMEAUTOMATIC:
					cout << "users\tOperation is resuming automatically from a low-power state." << endl;
					controller.resume();
					break;

				case PBT_APMRESUMESUSPEND:
					cout << "users\tOperation is resuming from a low-power state." << endl;
					controller.resume();
					break;

				case PBT_APMSUSPEND:
					cout << "users\tSystem is suspending operation." << endl;
					controller.sleep();
					break;

				default:
					cout << "users\tUnexpected power broadcast message (id=" << ((int) wParam) << ")" << endl;

				}

			} catch(const std::exception &e) {

				cerr << "users\tError '" << e.what() << "' processing WM_POWERBROADCAST" << endl;

			} catch(...) {

				cerr << "users\tUnexpected error processing WM_POWERBROADCAST" << endl;

			}
			break;

		case WM_WTSSESSION_CHANGE:

			try {

				switch((int) wParam) {
				case WTS_SESSION_LOCK:				// The session has been locked.
					{
						auto session = controller.find((DWORD) lParam);
						session->trace() << "WTS_SESSION_LOCK  (" << session->sid << ")" << endl;
						if(!session->flags.locked) {
							session->flags.locked = true;
							session->emit(lock);
						}
					}
					break;

				case WTS_SESSION_UNLOCK:			// The session identified has been unlocked.
					{
						auto session = controller.find((DWORD) lParam);
						session->trace() << "WTS_SESSION_UNLOCK  (" << session->sid << ")" << endl;
						if(session->flags.locked) {
							session->flags.locked = false;
							session->emit(unlock);
						}
					}
					break;

				case WTS_CONSOLE_CONNECT:			// The session was connected to the console terminal or RemoteFX session.
					{
						auto session = controller.find((DWORD) lParam);
						session->name(true);
						session->trace() << "WTS_CONSOLE_CONNECT  (" << session->sid << ")" << endl;
						session->set(User::SessionInForeground);
					}
					break;

				case WTS_REMOTE_CONNECT:			// The session was connected to the remote terminal.
					{
						auto session = controller.find((DWORD) lParam);
						session->name(true);
						session->trace() << "WTS_REMOTE_CONNECT  (" << session->sid << ")" << endl;
						session->flags.remote = true;
					}
					break;

				case WTS_REMOTE_DISCONNECT:			// The session was disconnected from the remote terminal.
					{
						auto session = controller.find((DWORD) lParam,false);
						if(session) {
							session->trace() << "WTS_REMOTE_DISCONNECT  (" << session->sid << ")" << endl;
							session->flags.remote = true;
							session->set(User::SessionIsClosing);
							{
								lock_guard<mutex> lock(controller.guard);
								controller.sessions.remove(session);
							}
						} else {
							Logger::trace() << "@" << ((DWORD) lParam) << "\tWTS_REMOTE_DISCONNECT" << endl;
						}
					}
					break;

				case WTS_CONSOLE_DISCONNECT:		// The session was disconnected from the console terminal or RemoteFX session.
					{
						auto session = controller.find((DWORD) lParam,false);
						if(session) {
							session->trace() << "WTS_CONSOLE_DISCONNECT  (" << session->sid << ")" << endl;
							session->flags.remote = false;
							session->set(User::SessionIsClosing);
							{
								lock_guard<mutex> lock(controller.guard);
								controller.sessions.remove(session);
							}
						} else {
							Logger::trace() << "@" << ((DWORD) lParam) << "\tWTS_CONSOLE_DISCONNECT" << endl;
						}
					}
					break;

				case WTS_SESSION_LOGON:				// A user has logged on to the session.
					{
						auto session = controller.find((DWORD) lParam);

						// Force username update.
						session->name(true);
						session->trace() << "WTS_SESSION_LOGON  (" << session->sid << ")" << endl;

						// Set to foreground on logon.
						session->set(User::SessionInForeground);
						session->emit(logon);
					}
					break;

				case WTS_SESSION_LOGOFF:			// A user has logged off the session.
					{
						auto session = controller.find((DWORD) lParam,false);
						if(session) {
							session->trace() << "WTS_SESSION_LOGOFF  (" << session->sid << ")" << endl;
							session->emit(logoff);
							session->set(User::SessionIsClosing);
							{
								lock_guard<mutex> lock(controller.guard);
								controller.sessions.remove(session);
							}
						} else {
							Logger::trace() << "@" << ((DWORD) lParam) << "\tWTS_SESSION_LOGOFF" << endl;
						}
					}
					break;

				case WTS_SESSION_REMOTE_CONTROL:	// The session has changed its remote controlled status.
					Logger::trace() << "@" << ((DWORD) lParam) << "\tWTS_SESSION_REMOTE_CONTROL" << endl;
					break;

				case WTS_SESSION_CREATE:			// Reserved for future use.
					Logger::trace() << "@" << ((DWORD) lParam) << "\tWTS_SESSION_CREATE" << endl;
					break;

				case WTS_SESSION_TERMINATE:			// Reserved for future use.
					Logger::trace() << "@" << ((DWORD) lParam) << "\tWTS_SESSION_TERMINATE" << endl;
					break;

				default:
					cerr	<< "@" << ((DWORD) lParam) << "\tWM_WTSSESSION_CHANGE sent an unexpected state '"
							<< ((int) wParam) << "'" << endl;
				}

			} catch(const std::exception &e) {

				cerr << "@" << ((DWORD) lParam) << "\t Error '" << e.what() << "' processing WM_WTSSESSION_CHANGE" << endl;

			} catch(...) {

				cerr << "@" << ((DWORD) lParam) << "\tUnexpected error processing WM_WTSSESSION_CHANGE" << endl;

			}
			break;

		case WM_CLOSE:
			cout << "users\tUser monitor window is closing (WM_CLOSE)" << endl;
			break;

		case WM_DESTROY:
			cout << "users\tUser monitor window was destroyed (WM_DESTROY)" << endl;
			if(controller.hwnd) {
				WTSUnRegisterSessionNotification(hWnd);
				controller.deinit();
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
