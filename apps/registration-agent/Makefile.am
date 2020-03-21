
EXTRA_DIST = registration-agent.init
EXTRA_DIST += registrationAgent.config
EXTRA_DIST += registration-agent.service
EXTRA_DIST += send-cmd.py
EXTRA_DIST += users.txt
EXTRA_DIST += mib2c-skeleton
EXTRA_DIST += README.txt
EXTRA_DIST += README_SNMP.txt

mibdir = $(datarootdir)/snmp/mibs
dist_mib_DATA = REG-AGENT-MIB.txt

sbin_PROGRAMS = registrationAgent

dist_man_MANS = registrationAgent.8

registrationAgent_SOURCES = AppSubsystem.cxx
registrationAgent_SOURCES += AppSubsystem.hxx
registrationAgent_SOURCES += CommandThread.cxx
registrationAgent_SOURCES += CommandThread.hxx
registrationAgent_SOURCES += KeyedFile.cxx
registrationAgent_SOURCES += KeyedFile.hxx
registrationAgent_SOURCES += RegConfig.cxx
registrationAgent_SOURCES += RegConfig.hxx
registrationAgent_SOURCES += registrationAgent.cxx
registrationAgent_SOURCES += SNMP_reSIProcate.cxx
registrationAgent_SOURCES += SNMP_reSIProcate.hxx
registrationAgent_SOURCES += SNMPThread.cxx
registrationAgent_SOURCES += SNMPThread.hxx
registrationAgent_SOURCES += UserAccount.cxx
registrationAgent_SOURCES += UserAccount.hxx
registrationAgent_SOURCES += UserRegistrationClient.cxx
registrationAgent_SOURCES += UserRegistrationClient.hxx

registrationAgent_LDADD = ../../resip/dum/libdum.la
registrationAgent_LDADD += ../../resip/stack/libresip.la
registrationAgent_LDADD += ../../rutil/librutil.la
registrationAgent_LDADD += -lssl -lcrypto -lpthread
registrationAgent_LDADD += -lqpid-proton-cpp
registrationAgent_LDADD += -ldl
registrationAgent_LDADD += @LIBNETSNMP_LDADD@



