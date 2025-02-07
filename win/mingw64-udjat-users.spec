#
# spec file for package mingw64-udjat-usersession
#
# Copyright (c) 2015 SUSE LINUX GmbH, Nuernberg, Germany.
# Copyright (C) <2008> <Banco do Brasil S.A.>
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via usersession://bugs.opensuse.org/
#

Summary:		User/Session module for udjat 
Name:			mingw64-udjat-users
Version: 2.0.0
Release:		0
License:		LGPL-3.0
Source:			udjat-module-users-%{version}.tar.xz

%define MAJOR_VERSION %(echo %{version} | cut -d. -f1)
%define MINOR_VERSION %(echo %{version} | cut -d. -f2 | cut -d+ -f1)
%define _libvrs %{MAJOR_VERSION}_%{MINOR_VERSION}

URL:			https://github.com/PerryWerneck/udjat-module-users

Group:			Development/Libraries/C and C++
BuildRoot:		/var/tmp/%{name}-%{version}

BuildRequires:	autoconf >= 2.61
BuildRequires:	automake
BuildRequires:	libtool
BuildRequires:	binutils
BuildRequires:	coreutils
BuildRequires:	gcc-c++

BuildRequires:	mingw64-cross-binutils
BuildRequires:	mingw64-cross-gcc
BuildRequires:	mingw64-cross-gcc-c++
BuildRequires:	mingw64-cross-pkg-config
BuildRequires:	mingw64-filesystem
BuildRequires:	mingw64-gettext-tools
BuildRequires:	mingw64-libudjat-devel

Requires:		mingw64-libudjatusers%{_libvrs}

%description
User/Session module for udjat

#---[ Library ]-------------------------------------------------------------------------------------------------------

%package -n mingw64-libudjatusers%{_libvrs}
Summary:	UDJat user session library
Requires:	mingw64-libudjat1_0

%description -n mingw64-libudjatusers%{_libvrs}
User session library for udjat

Simple user session abstraction library for udjat

#---[ Development ]---------------------------------------------------------------------------------------------------

%package -n mingw64-udjat-users-devel
Summary:	Development files for %{name}
Requires:	mingw64-libudjatusers%{_libvrs} = %{version}
Requires:	mingw64-libudjat-devel

%description -n mingw64-udjat-users-devel

Development files for Udjat's simple abstraction D-Bus library.


#---[ Build & Install ]-----------------------------------------------------------------------------------------------

%prep
%setup -n udjat-module-users-%{version}

NOCONFIGURE=1 \
	./autogen.sh

%{_mingw64_configure}

%build
make all %{?_smp_mflags}

%{_mingw64_strip} \
	--strip-all \
	.bin/Release/*.dll

%install
make DESTDIR=%{buildroot} install

%files
%defattr(-,root,root)
%{_mingw64_libdir}/udjat-modules/*.dll

%files -n mingw64-libudjatusers%{_libvrs}
%defattr(-,root,root)
%{_mingw64_bindir}/*.dll

%files -n mingw64-udjat-users-devel
%defattr(-,root,root)
%{_mingw64_includedir}/udjat/tools/*.h
%{_mingw64_libdir}/*.a
%{_mingw64_libdir}/pkgconfig/*.pc

%changelog

