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

 #include <config.h>
 #include <udjat/defs.h>
 #include <udjat/tools/usersession.h>
 #include <udjat/tools/mainloop.h>
 #include <system_error>
 #include <udjat/agent.h>
 #include <list>
 #include <memory>

 class Controller;

 /// @Brief Userlist agent.
 class UserList : public Udjat::Abstract::Agent {
 private:
	std::shared_ptr<::Controller> controller;

 public:
	UserList(const pugi::xml_node &node);
	virtual ~UserList();
	void onEvent(Udjat::User::Session &session, const Udjat::User::Event &event) noexcept;

 };

 /// @brief Singleton with the real userlist.
 class Controller : public Udjat::User::Controller, private Udjat::MainLoop::Service {
 private:
	static std::mutex guard;

 	/// @brief List of active userlist agents.
 	std::list<UserList *> agents;

 protected:

	std::shared_ptr<Udjat::User::Session> SessionFactory() noexcept override;

 public:
	Controller() = default;
	static std::shared_ptr<Controller> getInstance();

	void start() override;
	void stop() override;

	void insert(UserList *agent);
	void remove(UserList *agent);

	void for_each(std::function<void(UserList &agent)> callback);

 };
