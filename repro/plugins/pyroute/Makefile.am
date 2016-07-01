
AM_CXXFLAGS = -I $(top_srcdir)

reproplugin_LTLIBRARIES = libpyroute.la
libpyroute_la_SOURCES = PyRoutePlugin.cxx
libpyroute_la_SOURCES += PyRouteWorker.cxx
libpyroute_la_SOURCES += PyRouteProcessor.cxx
nodist_libpyroute_la_SOURCES = cxxextensions.c
nodist_libpyroute_la_SOURCES += cxx_extensions.cxx
nodist_libpyroute_la_SOURCES += cxxsupport.cxx
nodist_libpyroute_la_SOURCES += IndirectPythonInterface.cxx
libpyroute_la_CPPFLAGS = $(DEPS_PYTHON_CFLAGS)
libpyroute_la_LDFLAGS = -module -avoid-version
libpyroute_la_LDFLAGS += $(DEPS_PYTHON_LIBS)

EXTRA_DIST = example.py
EXTRA_DIST += example_ldap.py
EXTRA_DIST += README.txt

noinst_HEADERS = PyRouteWorker.hxx
noinst_HEADERS += PyThreadSupport.hxx
noinst_HEADERS += PyRouteProcessor.hxx

# Copy files from PyCXX into the tree to workaround an automake bug
# Discussed in the lists:
#   http://list.resiprocate.org/archive/resiprocate-devel/msg09110.html
#   http://lists.gnu.org/archive/html/automake/2016-06/msg00003.html
cxxextensions.c:	$(PYCXX_SRCDIR)/cxxextensions.c
	cp $(PYCXX_SRCDIR)/cxxextensions.c .

cxx_extensions.cxx:	$(PYCXX_SRCDIR)/cxx_extensions.cxx
	cp $(PYCXX_SRCDIR)/cxx_extensions.cxx .

cxxsupport.cxx:	$(PYCXX_SRCDIR)/cxxsupport.cxx
	cp $(PYCXX_SRCDIR)/cxxsupport.cxx .

IndirectPythonInterface.cxx:	$(PYCXX_SRCDIR)/../IndirectPythonInterface.cxx
	cp $(PYCXX_SRCDIR)/../IndirectPythonInterface.cxx .

