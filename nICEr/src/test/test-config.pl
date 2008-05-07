#!/usr/bin/perl

### DIRECTORIES:
###############

$NRAPPKIT_DIR = "/home/ekr/src/nrappkit";
$BUILD_SUBDIR = "linux-fedora";

$ICE_BUILD_SUBDIR = "linux-fedora-test";
$NRAPPKIT_BUILD_SUBDIR = $BUILD_SUBDIR;

$ICE_BINDIR = "../make/" . $ICE_BUILD_SUBDIR . "/";
$NRAPPKIT_BINDIR = $NRAPPKIT_DIR . "/src/make/" . $NRAPPKIT_BUILD_SUBDIR . "/";

$TEST_OUT_PREFIX = "./out/";   # Don't forget the trailing slash, if it's a dir

#### PROGRAMS:
#############

$REGISTRYD = $NRAPPKIT_BINDIR . "registryd";
$NRREGCTL = $NRAPPKIT_BINDIR . "nrregctl";


#### CONSTANTS:
##############

$PROGRAM_TIMEOUT = 120;
$OUT_CHECK_TIMEOUT = 10;


#### PLATFORM HANDLING:
######################

$IS_DARWIN  = 0;
$IS_LINUX   = 0;
$IS_FREEBSD = 0;
$IS_WIN32   = 0;

if ($BUILD_SUBDIR =~ "darwin") {
  $IS_DARWIN = 1;
}
elsif ($BUILD_SUBDIR =~ "freebsd") {
  $IS_FREEBSD = 1;
}
elsif ($BUILD_SUBDIR =~ "linux") {
  $IS_LINUX = 1;
}
elsif ($BUILD_SUBDIR =~ "win32") {
  $IS_WIN32 = 1;
}

1;

