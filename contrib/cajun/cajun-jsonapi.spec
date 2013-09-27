# cajun only ships headers, so no debuginfo package is needed
%define debug_package %{nil}

Summary: A cross-platform C++ header library for JSON
Name: cajun-jsonapi
Version: 2.0.3
Release: 1%{?dist}
URL: https://github.com/cajun-jsonapi/cajun-jsonapi
Source0: https://github.com/cajun-jsonapi/cajun-jsonapi/archive/%{version}.tar.gz
License: BSD
Group: System Environment/Libraries

%description
CAJUN is a C++ API for the JSON data interchange format with an emphasis
on an intuitive, concise interface. The library provides JSON types
and operations that mimic standard C++ as closely as possible in concept
and design.

%package devel
Group: Development/Libraries
Summary: Header files for cajun

%description devel
Header files you can use to develop applications with cajun.

CAJUN is a C++ API for the JSON data interchange format with an emphasis
on an intuitive, concise interface. The library provides JSON types
and operations that mimic standard C++ as closely as possible in concept
and design.

%prep
%setup -q

%build

%install
install -d -m755 $RPM_BUILD_ROOT/%{_includedir}/cajun/json
install -p -m644 json/* $RPM_BUILD_ROOT/%{_includedir}/cajun/json

%check
make %{?_smp_mflags}

%files devel
%defattr(-,root,root,-)
%doc Readme.txt ReleaseNotes.txt
%dir %{_includedir}/cajun
%dir %{_includedir}/cajun/json
%{_includedir}/cajun/json/*

%changelog
* Thu Sep 26 2013 Daniel Pocock <daniel@pocock.com.au> - 2.0.3-1
- Initial spec file
