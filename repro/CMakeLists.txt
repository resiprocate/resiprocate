file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/reproInfo.hxx" [=[#include "config.h"]=])

add_executable(repro
        repro.cxx
        )

target_link_libraries(repro PRIVATE librepro rutil)

#[[

EXTRA_DIST = *.html
EXTRA_DIST += doc
EXTRA_DIST += pkg
EXTRA_DIST += Doxyfile
EXTRA_DIST += php
EXTRA_DIST += README_MySQL.txt
EXTRA_DIST += README_PostgreSQL.txt
EXTRA_DIST += *.vcxproj *.vcxproj.filters
#EXTRA_DIST += *.db
EXTRA_DIST += VERSION
EXTRA_DIST += webadmin
EXTRA_DIST += WinSetup WinSetupx64
EXTRA_DIST += repro.config
EXTRA_DIST += create_mysql_reprodb.sql
EXTRA_DIST += create_postgresql_reprodb.sql
EXTRA_DIST += genUsers.cxx
EXTRA_DIST += users.txt

SUBDIRS = . test reprocmd accountingconsumers
if BUILD_REPRO_DSO_PLUGINS
SUBDIRS += plugins
endif

#AM_CXXFLAGS = -DUSE_ARES
AM_CXXFLAGS = -I $(top_srcdir)

sbin_PROGRAMS = repro
repro_SOURCES = repro.cxx
repro_LDADD = librepro.la
repro_LDADD += ../resip/dum/libdum.la
repro_LDADD += ../resip/stack/libresip.la
repro_LDADD += ../rutil/librutil.la
#repro_LDADD += ../contrib/ares/libares.a
#repro_LDADD += -L../contrib/ares -lares
repro_LDADD += @LIBRADIUS_LIBADD@
repro_LDADD += @LIBSSL_LIBADD@ @LIBPTHREAD_LIBADD@

lib_LTLIBRARIES = librepro.la

if BUILD_REPRO_DSO_PLUGINS
librepro_la_CXXFLAGS = -DREPRO_DSO_PLUGIN_DIR_DEFAULT='"@reproplugindir@"'
endif
librepro_la_LDFLAGS = @LIBTOOL_VERSION_RELEASE@ -export-dynamic
librepro_la_LIBADD = -ldb_cxx
librepro_la_LIBADD += ../resip/dum/libdum.la
librepro_la_LIBADD += ../resip/stack/libresip.la
librepro_la_LIBADD += ../rutil/librutil.la
if HAVE_LIBDL
librepro_la_LIBADD += -ldl
endif
]]

set(INCLUDES

        AbstractDb.hxx
        AccountingCollector.hxx
        Ack200DoneMessage.hxx
        AclStore.hxx
        AsyncProcessor.hxx
        AsyncProcessorMessage.hxx
        AsyncProcessorWorker.hxx
        AuthenticatorFactory.hxx
        BerkeleyDb.hxx
        BasicWsConnectionValidator.hxx
        CommandServer.hxx
        CommandServerThread.hxx
        ConfigStore.hxx
        FilterStore.hxx
        ForkControlMessage.hxx
        HttpBase.hxx
        HttpConnection.hxx
        monkeys/AmIResponsible.hxx
        monkeys/CertificateAuthenticator.hxx
        monkeys/CookieAuthenticator.hxx
        monkeys/ConstantLocationMonkey.hxx
        monkeys/DigestAuthenticator.hxx
        monkeys/GruuMonkey.hxx
        monkeys/IsTrustedNode.hxx
        monkeys/LocationServer.hxx
        monkeys/OutboundTargetHandler.hxx
        monkeys/QValueTargetHandler.hxx
        monkeys/RADIUSAuthenticator.hxx
        monkeys/RecursiveRedirect.hxx
        monkeys/SimpleStaticRoute.hxx
        monkeys/SimpleTargetHandler.hxx
        monkeys/StaticRoute.hxx
        monkeys/StrictRouteFixup.hxx
        monkeys/GeoProximityTargetSorter.hxx
        monkeys/RequestFilter.hxx
        monkeys/MessageSilo.hxx
        MySqlDb.hxx
        OutboundTarget.hxx
        PersistentMessageQueue.hxx
        Plugin.hxx
        PostgreSqlDb.hxx
        ProcessorChain.hxx
        Processor.hxx
        ProcessorMessage.hxx
        Proxy.hxx
        ProxyConfig.hxx
        QValueTarget.hxx
        Registrar.hxx
        RegSyncClient.hxx
        RegSyncServer.hxx
        RegSyncServerThread.hxx
        reproInfo.hxx
        ReproAuthenticatorFactory.hxx
        ReproRADIUSServerAuthManager.hxx
        ReproServerAuthManager.hxx
        ReproTlsPeerAuthManager.hxx
        ReproRunner.hxx
        ReproVersion.hxx
        RequestContext.hxx
        ResponseContext.hxx
        RouteStore.hxx
        RRDecorator.hxx
        SiloStore.hxx
        SqlDb.hxx
        stateAgents/CertPublicationHandler.hxx
        stateAgents/CertServer.hxx
        stateAgents/CertSubscriptionHandler.hxx
        stateAgents/PresencePublicationHandler.hxx
        stateAgents/PresenceServer.hxx
        stateAgents/PresenceSubscriptionHandler.hxx
        stateAgents/PrivateKeyPublicationHandler.hxx
        stateAgents/PrivateKeySubscriptionHandler.hxx
        StaticRegStore.hxx
        Store.hxx
        Target.hxx
        TimerCMessage.hxx
        TlsPeerIdentityInfo.hxx
        TlsPeerIdentityStore.hxx
        UserAuthGrabber.hxx
        UserInfoMessage.hxx
        UserStore.hxx
        WebAdmin.hxx
        WebAdminThread.hxx
        XmlRpcConnection.hxx
        XmlRpcServerBase.hxx
        )

add_library(librepro
RouteStore.cxx
UserStore.cxx
ConfigStore.cxx
AclStore.cxx
StaticRegStore.cxx
FilterStore.cxx
SiloStore.cxx
Store.cxx
AbstractDb.cxx
BerkeleyDb.cxx
BasicWsConnectionValidator.cxx

CommandServer.cxx
CommandServerThread.cxx
ProxyConfig.cxx
ReproVersion.cxx
HttpBase.cxx
HttpConnection.cxx
WebAdmin.cxx
WebAdminThread.cxx

AccountingCollector.cxx
Proxy.cxx
Registrar.cxx
RegSyncClient.cxx
RegSyncServer.cxx
RegSyncServerThread.cxx
ReproRunner.cxx
ReproAuthenticatorFactory.cxx
ReproRADIUSServerAuthManager.cxx
ReproServerAuthManager.cxx
ReproTlsPeerAuthManager.cxx
RequestContext.cxx
ResponseContext.cxx
RRDecorator.cxx
Processor.cxx
ProcessorChain.cxx
SqlDb.cxx
Target.cxx
TlsPeerIdentityStore.cxx
UserAuthGrabber.cxx
XmlRpcConnection.cxx
XmlRpcServerBase.cxx
OutboundTarget.cxx
PersistentMessageQueue.cxx
QValueTarget.cxx

stateAgents/PresenceServer.cxx
stateAgents/PresencePublicationHandler.cxx
stateAgents/PresenceSubscriptionHandler.cxx
monkeys/CertificateAuthenticator.cxx
monkeys/DigestAuthenticator.cxx
monkeys/CookieAuthenticator.cxx
monkeys/StrictRouteFixup.cxx
monkeys/AmIResponsible.cxx
monkeys/IsTrustedNode.cxx
monkeys/ConstantLocationMonkey.cxx
monkeys/LocationServer.cxx
monkeys/OutboundTargetHandler.cxx
monkeys/RADIUSAuthenticator.cxx
monkeys/RecursiveRedirect.cxx
monkeys/SimpleStaticRoute.cxx
monkeys/StaticRoute.cxx
monkeys/QValueTargetHandler.cxx
monkeys/SimpleTargetHandler.cxx
monkeys/GeoProximityTargetSorter.cxx
monkeys/RequestFilter.cxx
monkeys/MessageSilo.cxx

${INCLUDES}

        )

target_include_directories(librepro PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(librepro PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(librepro PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_include_directories(librepro PRIVATE ${CAJUN_INCLUDE_DIRS})

#[[
reproincludedir = $(includedir)/repro
nobase_reproinclude_HEADERS =


if USE_SSL
librepro_la_SOURCES +=
stateAgents/CertServer.cxx
stateAgents/CertPublicationHandler.cxx
stateAgents/CertSubscriptionHandler.cxx
stateAgents/PrivateKeyPublicationHandler.cxx
stateAgents/PrivateKeySubscriptionHandler.cxx
endif

if USE_MYSQL
librepro_la_SOURCES += MySqlDb.cxx
librepro_la_LIBADD += @LIBMYSQL_LIBADD@
endif

if USE_POSTGRESQL
librepro_la_SOURCES += PostgreSqlDb.cxx
librepro_la_LIBADD += @LIBPOSTGRESQL_LIBADD@
endif

if USE_MAXMIND_GEOIP
librepro_la_LIBADD += @LIBGEOIP_LIBADD@
endif

if BUILD_QPID_PROTON
librepro_la_SOURCES += QpidProtonThread.cxx
nobase_reproinclude_HEADERS += QpidProtonThread.hxx
librepro_la_LIBADD += -lqpid-proton-cpp
endif

# reproInfo.hxx is created manually be other build systems
# but here we just delegate to the autoconf generated config.h
reproInfo.hxx:
@echo "#include"config.h\"" >  reproInfo.hxx
cat reproInfo.hxx

ReproVersion.cxx: reproInfo.hxx

dist_man_MANS = doc/repro.8
dist_man_MANS += doc/reprocmd.8

]]
