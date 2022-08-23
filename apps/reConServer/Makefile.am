
EXTRA_DIST = reConServer_readme.txt
EXTRA_DIST += reConServer.config
EXTRA_DIST += pkg
EXTRA_DIST += *.wav
EXTRA_DIST += reconserver.init
EXTRA_DIST += reconserver.service

if BUILD_RECONSERVER
sbin_PROGRAMS = reConServer
endif

dist_man_MANS = reConServer.8

reConServer_SOURCES = reConServer.cxx
reConServer_SOURCES += reConServer.hxx
reConServer_SOURCES += reConServerConfig.cxx
reConServer_SOURCES += reConServerConfig.hxx
reConServer_SOURCES += AppSubsystem.cxx
reConServer_SOURCES += AppSubsystem.hxx
reConServer_SOURCES += B2BCallManager.cxx
reConServer_SOURCES += B2BCallManager.hxx
reConServer_SOURCES += CDRFile.cxx
reConServer_SOURCES += CDRFile.hxx
reConServer_SOURCES += CredentialGrabber.cxx
reConServer_SOURCES += CredentialGrabber.hxx
reConServer_SOURCES += CredentialInfo.cxx
reConServer_SOURCES += CredentialInfo.hxx
reConServer_SOURCES += MyConversationManager.cxx
reConServer_SOURCES += MyConversationManager.hxx
reConServer_SOURCES += MyMessageDecorator.cxx
reConServer_SOURCES += MyMessageDecorator.hxx
reConServer_SOURCES += MyUserAgent.cxx
reConServer_SOURCES += MyUserAgent.hxx
reConServer_SOURCES += RegistrationForwarder.cxx
reConServer_SOURCES += RegistrationForwarder.hxx
reConServer_SOURCES += SubscriptionForwarder.cxx
reConServer_SOURCES += SubscriptionForwarder.hxx
reConServer_SOURCES += playback_prompt.h
reConServer_SOURCES += record_prompt.h
if BUILD_QPID_PROTON
reConServer_SOURCES += \
        ProtonCommandThread.cxx \
        ProtonCommandThread.hxx
endif

reConServer_LDADD = ../../resip/recon/librecon.la
reConServer_LDADD += ../../reflow/libreflow.la
if BUILD_QPID_PROTON
reConServer_LDADD += -lqpid-proton-cpp
endif
if USE_SIPXTAPI
reConServer_LDADD += -lsipXsdp
reConServer_LDADD += -lsipXmediaProcessing
reConServer_LDADD += -lsipXmedia
reConServer_LDADD += -lsipXport
reConServer_LDADD += -lsipXtack
endif
if USE_KURENTO
reConServer_LDADD += -lboost_system
reConServer_LDADD += ../../media/kurento/libkurentoclient.la
endif
reConServer_LDADD += ../../resip/dum/libdum.la
reConServer_LDADD += ../../resip/stack/libresip.la
reConServer_LDADD += ../../rutil/librutil.la
reConServer_LDADD += -lssl -lcrypto -lpthread
reConServer_LDADD += -lsoci_core -ldl
if USE_SOCI_POSTGRESQL
reConServer_LDADD += -lsoci_postgresql
endif
if USE_SOCI_MYSQL
reConServer_LDADD += -lsoci_mysql -lmysqlclient
endif
reConServer_LDADD += -lfmt

BUILT_SOURCES = playback_prompt.h record_prompt.h

playback_prompt.h:
	echo "#ifndef _playback_prompt_h_" > playback_prompt.h
	echo "#define _playback_prompt_h_" >> playback_prompt.h
	xxd -i -u playback_prompt.wav >> playback_prompt.h
	sed  -i 's/_wav//g' playback_prompt.h
	echo "#endif" >> playback_prompt.h

record_prompt.h:
	echo "#ifndef _record_prompt_h_" > record_prompt.h
	echo "#define _record_prompt_h_" >> record_prompt.h
	xxd -i -u record_prompt.wav >> record_prompt.h
	sed  -i 's/_wav//g' record_prompt.h
	echo "#endif" >> record_prompt.h


