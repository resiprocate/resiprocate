Name:     resiprocate
Version:  1.1
Release:  1

Summary:  Resiprocate SIP Stack
License:  Vovida Software License http://opensource.org/licenses/vovidapl.php
Group:    Productivity/Telephony/SIP/Servers
Vendor:   resiprocate.org
Packager: Alfred E. Heggestad <aeh@db.org>
Url:      http://www.resiprocate.org

Source:   %name-%version.tar.gz

BuildRequires: openssl-devel >= 0.9.7
BuildRequires: popt
BuildRequires: boost-devel

Requires: openssl >= 0.9.7
Requires: chkconfig

Prefix:    %_prefix
BuildRoot: %{_tmppath}/%name-%version-root

%description
The reSIProcate components, particularly the SIP stack, are in use in both
commercial and open-source products. The project is dedicated to maintaining
a complete, correct, and commercially usable implementation of SIP and a few
related protocols.

%package devel
Summary:        Resiprocate development files
Group:          Development/Libraries
Requires:       %{name} = %{version}

%description devel
Resiprocate SIP Stack development files.

%prep
%setup -q

%build
./configure -y --with-compile-type=opt --enable-shared-libs --disable-ssl --disable-sigcomp --disable-ipv6 --prefix=/usr --ares-prefix=/usr
make resiprocate

%install
# makeinstall RPM macro doesn't leverage DESTDIR but instead overrides
# libdir, bindir, etc just for make install. This not copesetic w/how
# our makefiles are built, they'd rather preserve libdir, and use
# DESTDIR when copying/moving/creating files.  The approach we're taking
# is quite standard, so it's surprising RPM's makeinstall macro is
# the way it is.
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT INSTALL_PREFIX=/usr install-rutil install-resip
make DESTDIR=$RPM_BUILD_ROOT ARES_PREFIX=/usr install-ares

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%defattr(644,root,root,755)
%{_libdir}/librutil.so
%{_libdir}/libresip.so

%files devel
%defattr(644,root,root,755)
%{_includedir}/rutil/*.hxx
%{_includedir}/rutil/dns/*.hxx
%{_includedir}/rutil/stun/*.hxx
%{_includedir}/resip/stack/*.hxx
%{_includedir}/resip/stack/config.hxx.in
#%{_libdir}/librutil.a
#%{_libdir}/libresip.a
%{_libdir}/libares.a
#%{_mandir}/man3/ares*gz
/usr/man/man3/ares*.gz

%changelog
* Thu May 24 2007 Alfred E. Heggestad <aeh@db.org> -
- Initial build.

