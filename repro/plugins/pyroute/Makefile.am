
AM_CXXFLAGS = -I $(top_srcdir)

PYCXX_SRCDIR = /usr/share/python2.7/CXX/Python2

reproplugin_LTLIBRARIES = libpyroute.la
libpyroute_la_SOURCES = PyRoutePlugin.cxx
libpyroute_la_SOURCES += $(PYCXX_SRCDIR)/cxxextensions.c
libpyroute_la_SOURCES += $(PYCXX_SRCDIR)/cxx_extensions.cxx
libpyroute_la_SOURCES += $(PYCXX_SRCDIR)/cxxsupport.cxx
libpyroute_la_SOURCES += $(PYCXX_SRCDIR)/../IndirectPythonInterface.cxx
libpyroute_la_CPPFLAGS = $(DEPS_PYTHON_CFLAGS)
libpyroute_la_LDFLAGS = -module -avoid-version
libpyroute_la_LDFLAGS += $(DEPS_PYTHON_LIBS)

EXTRA_DIST = example.py

