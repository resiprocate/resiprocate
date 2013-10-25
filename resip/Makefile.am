
EXTRA_DIST = README.windows
EXTRA_DIST += sip.mcp.xml
EXTRA_DIST += update-svn-ignores.sh

SUBDIRS = stack dum certs
if BUILD_REND
SUBDIRS += rend
endif

# recon has a lot of dependencies, e.g. sipX, so it is not yet
# adapted for autotools
#
#SUBDIRS += recon

