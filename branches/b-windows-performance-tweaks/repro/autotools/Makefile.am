AUTOMAKE_OPTIONS=foreign

SUBDIRS= . 

CXX = ${DISTCC} @CXX@

LIBTOOL = @LIBTOOL@ --tag=CXX


bin_PROGRAMS=repro

repro_SOURCES= AbstractDb.cxx \
               AclStore.cxx \
               BerkeleyDb.cxx \
               CommandLineParser.cxx \
               ConfigStore.cxx \
               HttpBase.cxx \
               HttpConnection.cxx \
               monkeys/AmIResponsible.cxx \
               monkeys/ConstantLocationMonkey.cxx \
               monkeys/DigestAuthenticator.cxx \
               monkeys/IsTrustedNode.cxx \
               monkeys/LocationServer.cxx \
               monkeys/StaticRoute.cxx \
               monkeys/StrictRouteFixup.cxx \
               MySqlDb.cxx \
               Proxy.cxx \
               Registrar.cxx \
               repro.cxx \
               ReproServerAuthManager.cxx \
               RequestContext.cxx \
               RequestProcessorChain.cxx \
               RequestProcessor.cxx \
               ResponseContext.cxx \
               RouteStore.cxx \
               stateAgents/CertPublicationHandler.cxx \
               stateAgents/CertServer.cxx \
               stateAgents/CertSubscriptionHandler.cxx \
               stateAgents/PrivateKeyPublicationHandler.cxx \
               stateAgents/PrivateKeySubscriptionHandler.cxx \
               Store.cxx \
               UserStore.cxx \
               WebAdmin.cxx \
               WebAdminThread.cxx 
               
#This is awful, and the code should be changed to make it go away
INCLUDES = -I..
