
AM_CXXFLAGS = -I $(top_srcdir)

lib_LTLIBRARIES = libresipdb.la

libresipdb_la_LIBADD = -ldb_cxx
libresipdb_la_LIBADD += ../../rutil/librutil.la

libresipdb_la_SOURCES = \
        RouteStore.cxx \
        UserStore.cxx \
        ConfigStore.cxx \
        AclStore.cxx \
    StaticRegStore.cxx \
        FilterStore.cxx \
        SiloStore.cxx \
        Store.cxx \
        AbstractDb.cxx \
        BerkeleyDb.cxx

resipdbincludedir = $(includedir)/resip/db
nobase_resipdbinclude_HEADERS = AbstractDb.hxx \
	AclStore.hxx \
	BerkeleyDb.hxx \
	ConfigStore.hxx \
	FilterStore.hxx \
	MySqlDb.hxx \
	RouteStore.hxx \
	SiloStore.hxx \
	StaticRegStore.hxx \
	Store.hxx \
	UserStore.hxx

