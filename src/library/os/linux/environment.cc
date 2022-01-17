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
 #include <udjat/tools/usersession.h>
 #include <dirent.h>
 #include <systemd/sd-login.h>
 #include <cstdlib>
 #include <sys/stat.h>
 #include <fcntl.h>
 #include <unistd.h>
 #include <cstring>
 #include <udjat/tools/file.h>

 using namespace std;

 namespace Udjat {

	/// @brief File on /proc/[PID]/environ
	class Environ {
	private:
		int descriptor = -1;

	public:
		Environ(DIR * dir, const char *name) : descriptor(openat(dirfd(dir),(string{name} + "/environ").c_str(),O_RDONLY)) {
		}

		~Environ() {

			if(descriptor > 0) {
				::close(descriptor);
			}
		}

		operator bool() const {
			return descriptor >= 0;
		}

		uid_t uid() {
			struct stat st;
			if(fstat(descriptor,&st) < 0)
				return (uid_t) -1;
			return st.st_uid;
		}

		int fd() {
			return this->descriptor;
		}

	};


	string User::Session::getenv(const char *varname) const {

		// This is designed to find the user's dbus-session allowing
		// subscription of user's session signals from system daemons
		// to monitor gnome session locks/unlocks.

		// This would be far more easier with the fix of the issue
		// https://gitlab.gnome.org/GNOME/gnome-shell/-/issues/741#

		string value;
		size_t szName = strlen(varname);
		uid_t uid = this->userid();

		// https://stackoverflow.com/questions/6496847/access-another-users-d-bus-session
		DIR * dir = opendir("/proc");
        if(!dir) {
                throw std::system_error(errno, std::system_category());
        }

        try {

			struct dirent *ent;
			while((ent=readdir(dir))!=NULL) {

				Environ environ(dir,ent->d_name);

				if(!environ || environ.uid() != uid) {
					continue;
				}

				// Check if it's the required session
				{
					char *sname = nullptr;

					// Se o processo não for associado a uma sessão ignora.
					if(sd_pid_get_session(atoi(ent->d_name), &sname) == -ENODATA || !sname)
						continue;

					int sval = strcmp(sid.c_str(),sname);
					free(sname);

					if(sval)
						continue;
				}


				// It's an user environment, scan it.
				{
					File::Text text(environ.fd());
					for(const char *ptr = text.c_str(); *ptr; ptr += (strlen(ptr)+1)) {
						if(strncmp(ptr,varname,szName) == 0 && ptr[szName] == '=') {
							value.assign(ptr+szName+1);
							break;
						}
					}
				}
			}

        } catch(...) {

			closedir(dir);
			throw;

        }

		closedir(dir);

		return value;

	}

 }
