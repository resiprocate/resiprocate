AUTOMAKE_OPTIONS=foreign

SUBDIRS= . 

CXX = ${DISTCC} @CXX@

bin_PROGRAMS=repro
repro_SOURCES= repro.cxx \
               RouteStore.cxx \
               UserStore.cxx \
               ConfigStore.cxx \
               AclStore.cxx \
               Store.cxx \
               AbstractDb.cxx \
               BerkeleyDb.cxx \
               CommandLineParser.cxx \
               HttpBase.cxx \
               HttpConnection.cxx \
               WebAdmin.cxx \
               WebAdminThread.cxx \
               Proxy.cxx \
               Registrar.cxx \
               ReproServerAuthManager.cxx \
               RequestContext.cxx \
               ResponseContext.cxx \
               RequestProcessor.cxx \
               RequestProcessorChain.cxx \
               monkeys/DigestAuthenticator.cxx \
               monkeys/RouteProcessor.cxx \
               monkeys/AmIResponsible.cxx \
               monkeys/LocationServer.cxx \
               monkeys/ConstantLocationMonkey.cxx \
               monkeys/RouteMonkey.cxx \
               stateAgents/CertServer.cxx \
               stateAgents/CertPublicationHandler.cxx \
               stateAgents/CertSubscriptionHandler.cxx \
               stateAgents/PrivateKeyPublicationHandler.cxx \
               stateAgents/PrivateKeySubscriptionHandler.cxx \
			   MySqlDb.cxx
