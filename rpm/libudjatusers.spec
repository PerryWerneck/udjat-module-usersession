#
# spec file for package libudjatusers
#
# Copyright (c) <2024> Perry Werneck <perry.werneck@gmail.com>.
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via https://github.com/PerryWerneck/libudjatusers/issues
#

%define module_name users

Summary:		User/Session library for %{product_name}  
Name:			libudjat%{module_name}
Version: 1.2.0
Release:		0
License:		LGPL-3.0
Source:			%{name}-%{version}.tar.xz

URL:			https://github.com/PerryWerneck/libudjat%{module_name}

Group:			Development/Libraries/C and C++
BuildRoot:		/var/tmp/%{name}-%{version}

BuildRequires:	binutils
BuildRequires:	coreutils

BuildRequires:	gcc-c++ >= 5
BuildRequires:	pkgconfig(libudjat)
BuildRequires:	pkgconfig(libudjatdbus)
BuildRequires:	pkgconfig(libsystemd)
BuildRequires:  meson

%description
User/Session library for %{product_name}

User session classes for use with lib%{product_name}

#---[ Library ]-------------------------------------------------------------------------------------------------------

%package -n %{udjat_library}
Summary: User/Session library for %{product_name}

%lang_package -n %{udjat_library}
User/Session library for %{product_name}

C++ user session classes for use with lib%{product_name}

#---[ Development ]---------------------------------------------------------------------------------------------------

%package devel
Summary: Development files for %{name}
%udjat_devel_requires

%description devel
User/Session client library for %{product_name}

C++ user session classes for use with lib%{product_name}

%lang_package -n %{name}%{_libvrs}
%udjat_module_package -n users

%prep
%autosetup
%meson

%build
%meson_build

%install
%meson_install
%find_lang %{name}-%{udjat_major}.%{udjat_minor} langfiles

%files -n %{udjat_library}
%defattr(-,root,root)
%{_libdir}/%{name}.so.%{udjat_major}.%{udjat_minor}

%files -n %{udjat_library}-lang -f langfiles

%files devel
%defattr(-,root,root)

%{_libdir}/*.so
%{_libdir}/*.a
%{_libdir}/pkgconfig/*.pc
%{_includedir}/udjat/*/*.h

%dir %{_includedir}/udjat/tools/user
%{_includedir}/udjat/tools/user/*.h

%post -n %{name}%{_libvrs} -p /sbin/ldconfig

%postun -n %{name}%{_libvrs} -p /sbin/ldconfig

%changelog

