#
# spec file for package mingw64-udjat-civetweb
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

# Please submit bugfixes or comments via 
#

%define udjat_version %(x86_64-w64-mingw32-pkg-config --modversion libudjat | tr "." "_")
%define product %(x86_64-w64-mingw32-pkg-config --variable product_name libudjat)
%define moduledir %(x86_64-w64-mingw32-pkg-config --variable module_path libudjat)

Summary:		Windows http server for %{product}
Name:			mingw64-udjat-civetweb
Version:		1.0
Release:		0
License:		LGPL-3.0
Source:			udjat-module-civetweb-%{version}.tar.xz

URL:			https://github.com/PerryWerneck/udjat-module-civetweb

Group:			Development/Libraries/C and C++
BuildRoot:		/var/tmp/%{name}-%{version}

%define MAJOR_VERSION %(echo %{version} | cut -d. -f1)
%define MINOR_VERSION %(echo %{version} | cut -d. -f2 | cut -d+ -f1)
%define _libvrs %{MAJOR_VERSION}_%{MINOR_VERSION}

BuildArch:		noarch

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
BuildRequires:	mingw64-civetweb-devel

Requires:		mingw64-udjathttpd%{_libvrs} = %{version}

%description
HTTP Exporter module for udjat

#---[ Library ]-------------------------------------------------------------------------------------------------------

%package -n mingw64-udjathttpd%{_libvrs}
Summary:	UDJat httpd library
Requires:	mingw64-libudjat1_0

%description -n mingw64-udjathttpd%{_libvrs}
HTTP Server abstraction library for udjat

#---[ Development ]---------------------------------------------------------------------------------------------------

%package -n mingw64-udjat-httpd-devel
Summary:	Development files for %{name}
Requires:	pkgconfig(libudjat)
Requires:	mingw64-udjathttpd%{_libvrs} = %{version}

%description -n mingw64-udjat-httpd-devel

Development files for Udjat's HTTP server abstraction library.

#---[ Build & Install ]-----------------------------------------------------------------------------------------------

%prep
%setup -n udjat-module-civetweb-%{version}

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
%{moduledir}/*.dll

%files -n mingw64-udjathttpd%{_libvrs}
%defattr(-,root,root)
%{_mingw64_bindir}/*.dll
%{_mingw64_datadir}/locale/*/LC_MESSAGES/*.mo

%files -n mingw64-udjat-httpd-devel
%defattr(-,root,root)
%{_mingw64_includedir}/udjat/tools/http/*.h
%{_mingw64_libdir}/*.a
%{_mingw64_libdir}/pkgconfig/*.pc

%changelog

