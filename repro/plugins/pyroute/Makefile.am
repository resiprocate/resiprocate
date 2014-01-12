
AM_CXXFLAGS = -I $(top_srcdir)

reproplugin_LTLIBRARIES = libpyroute.la
libpyroute_la_SOURCES = PyRoutePlugin.cxx
libpyroute_la_SOURCES += PyRouteWorker.cxx
libpyroute_la_SOURCES += PyRouteProcessor.cxx
nodist_libpyroute_la_SOURCES = $(PYCXX_SRCDIR)/cxxextensions.c
nodist_libpyroute_la_SOURCES += $(PYCXX_SRCDIR)/cxx_extensions.cxx
nodist_libpyroute_la_SOURCES += $(PYCXX_SRCDIR)/cxxsupport.cxx
nodist_libpyroute_la_SOURCES += $(PYCXX_SRCDIR)/../IndirectPythonInterface.cxx
libpyroute_la_CPPFLAGS = $(DEPS_PYTHON_CFLAGS)
libpyroute_la_LDFLAGS = -module -avoid-version
libpyroute_la_LDFLAGS += $(DEPS_PYTHON_LIBS)

EXTRA_DIST = example.py
EXTRA_DIST += example_ldap.py
EXTRA_DIST += README.txt

noinst_HEADERS = PyRouteWorker.hxx
noinst_HEADERS += PyThreadSupport.hxx
noinst_HEADERS += PyRouteProcessor.hxx

