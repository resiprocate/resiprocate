
AM_CXXFLAGS = -I $(top_srcdir)

reproplugin_LTLIBRARIES = libpyroute.la
libpyroute_la_SOURCES = PyRoutePlugin.cxx
libpyroute_la_SOURCES += PyRouteWorker.cxx
libpyroute_la_SOURCES += PyRouteProcessor.cxx
libpyroute_la_CPPFLAGS = $(DEPS_PYTHON_CFLAGS)
libpyroute_la_LDFLAGS = -module -avoid-version
libpyroute_la_LDFLAGS += $(DEPS_PYTHON_LIBS)
# this isn't included in the output from python3-config --ldflags
# so we set it explicitly
libpyroute_la_LDFLAGS += -lpython$(DEPS_PYTHON_VERSION)

EXTRA_DIST = example.py
EXTRA_DIST += example_ldap.py
EXTRA_DIST += README.txt

noinst_HEADERS = PyRouteWorker.hxx
noinst_HEADERS += PyRouteProcessor.hxx

