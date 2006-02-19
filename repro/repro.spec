Name:     repro
Version:  @VERSION@
Release:  %{buildno}

Summary:  Repro SIP Proxy
License:  Vovida Software License http://opensource.org/licenses/vovidapl.php
Group:    Productivity/Telephony/SIP/Servers
Vendor:   SIPfoundry
Packager: SIPfoundry <repro-devel@list.sipfoundry.org>
Url:      http://www.sipfoundry.org/repro

Source:   %name-%version.tar.gz

BuildRequires: openssl-devel >= 0.9.7
BuildRequires: db4-devel
BuildRequires: popt
BuildRequires: boost-devel

Requires: openssl >= 0.9.7
Requires: chkconfig

Prefix:    %_prefix
BuildRoot: %{_tmppath}/%name-%version-root

%description
The repro SIP server provides SIP proxy, registrar, redirect and identity
services. It can be used as the central rendezvous service for peer-to-peer
voice, IM, and presence services, as the core of large scale internet
telephony services, and as a tool to enforce policy at the boundary between
networks or domains.

%prep
%setup -q

%build
./configure -y --prefix=/usr
make repro

%install
# makeinstall RPM macro doesn't leverage DESTDIR but instead overrides
# libdir, bindir, etc just for make install. This not copesetic w/how
# our makefiles are built, they'd rather preserve libdir, and use
# DESTDIR when copying/moving/creating files.  The approach we're taking
# is quite standard, so it's surprising RPM's makeinstall macro is
# the way it is.
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install-repro
make -C repro DESTDIR=$RPM_BUILD_ROOT install-rh-config

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(644,root,root,755)
%attr(750,root,@REPROUSER@) %{_sbindir}/repro
%attr(750,root,@REPROUSER@) %{_sysconfdir}/init.d/repro
%{_mandir}/man8/repro.8.gz

# The configuration directory needs to be writeable because
# the configuration web interface modifies them.
%config(noreplace) %attr(644,root,root) %{_sysconfdir}/repro.conf

# the directory where repro is executed and stores its configuration files
%dir %attr(750,@REPROUSER@,@REPROUSER@) @REPRO_CWD@
# the directory where repro puts its pid file
%dir %attr(770,root,@REPROUSER@) @REPRO_RUNDIR@

%pre
id --user @REPROUSER@ > /dev/null 2>&1 || /usr/sbin/useradd -M -c "repro sip proxy daemon" -r @REPROUSER@

%post
# Arrange for sipX to be started every time the system starts up.
# It starts in runlevels 3 and 5.
chkconfig --add repro
