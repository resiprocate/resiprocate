###############################################################################
# build.py
# by Sailesh@xten.com
#
# Usage: python build.py
#
# A Python script to build unit tests on the Mac using command line tools.
# Currently this script will not work if you are logged in remotely
# using SSH. This can be fixed by removing dependencies to cmdide.
#
# Setup:
#
# This script requires having CodeWarrior's command line tools installed.
# Follow the instructions in "Command Line Tools" directory of your
# CodeWarrior installation folder.
#
# The install script assumes that your shell is tcsh. If you're using
# bash, the standard shell on OS X, then you'll need to convert the
# file mwvars to using bash syntax.
#
# Next, in your .tcshrc or .bashrc file, change "source mwvars" to
# "source mwvars bsd". This causes the command line tools to use
# BSD headers and libraries.
#
# Finally, there is one bug in the mwvars file we have to fix.
# For the environment variable MWCMacOSXPPCIncludes, append:
#   "$CWINSTALL"/MacOS\ X\ Support/Headers/PPC\ Specific
#
###############################################################################

import os
import sys

# GLOBAL VARIABLES
IDE_PATH = None       # path to CodeWarrior IDE
cpp_compiler = None   # path to c++ compiler
proj_compiler = None  # path to ide compiler, it compiles .mcp files
cpp_options = None    # options to mwcc, c++ compiler
ld_options = None     # options to mwld, linker

###############################################################################
# sets the path to the c++ compiler and setup up any environment variables
###############################################################################
def setup_environment():
    global IDE_PATH
    global cpp_compiler 
    global proj_compiler

    # make sure we know where the CodeWarrior IDE is
    IDE_PATH = os.environ.get("CWINSTALL")
    if IDE_PATH == None:
        print "Error: unable to locate CodeWarrior IDE install location."
        print "Please follow the setup directions at the top of this script."
        sys.exit(1)

    # add the compiler to the path
    cpp_compiler_path = IDE_PATH + "/../Other Metrowerks Tools/Command Line Tools"
    old_path = os.environ['PATH']
    os.environ['PATH'] = old_path + ":" + cpp_compiler_path

    # setup an explicit path to mwcc, the c++ compiler
    cpp_compiler = "\"" + cpp_compiler_path + "/mwcc" + "\"" 

    # setup an explicit path to cmdide, the compiler that takes a .mcp file
    proj_compiler = "\"" + cpp_compiler_path + "/cmdide" + "\"" 

###############################################################################
# setup compiler and linker options
###############################################################################
def setup_options():
    global cpp_options
    global ld_options

    # enable wchar_t support
    cpp_options =  " -wchar_t on"

    # adopt GCC #include semantics
    cpp_options += " -gccinc"

    # add sip directory to our include search path
    cpp_options += " -I../../.."

    # prefix file to set some macros
    cpp_options += " -I."
    cpp_options += " -include prefix.h"

    # link against ares and resiprocate
    # see build_dependencies for more info
    ld_options   = " -l../../../Output/aresdebug.lib"
    ld_options  += " -l../../../Output/resiprocatedebug.lib"

    # security libraries
    ld_options  += " -l /usr/local/lib"
    ld_options  += " -llibssl.a"
    ld_options  += " -llibcrypto.a"

    # system libraries
    ld_options  += " -llibgcc.a"
    ld_options  += " -lMSL_All_Mach-O_D.lib"

    # Mac OS X frameworks
    ld_options += " -framework CoreFoundation"
    ld_options += " -framework Security"

###############################################################################
# builds ares and resiprocate if they're not already there
###############################################################################
def build_dependencies():
    global proj_compiler

    # we use cmdide to build .mcp files
    # the path to cmdide is created in setup_environment()
    if proj_compiler == None:
        print "Error: need to call setup_environment first"
        sys.exit(1)

    # command line options
    # -b => build target, -q => quit IDE after building
    # -z => switch to named target

    # NOTE: If we put "-q" when we build the first target then
    # there may be a problem building the 2nd target. This is because
    # the command line tool tries to open the 2nd target before the 1st IDE
    # has finished.

    os.system(proj_compiler + " -proj -z ResiprocateDebug -b ../../../sip.mcp")
    os.system(proj_compiler + " -proj -z AresDebug -b -q ../../../sip.mcp")

###############################################################################
# build each test program
# output file name is always "input_source.out"
###############################################################################
def build_tests(test_programs):
    global cpp_options
    global ld_options
    global cpp_compiler

    # set up the compiler
    setup_environment()
    setup_options()
    build_dependencies()

    # invoke the compiler
    for curTest in test_programs:
        outputName = curTest + ".out"
        options = cpp_options + ld_options + " -o " + outputName
        os.system(cpp_compiler + options + " " + curTest)

###############################################################################
# removes .o and .out files from sip/resiprocrate/test
###############################################################################
def run_tests(test_programs):
    for curTest in test_programs:
        outputName = curTest + ".out"
        # TODO: execute outputName and check if test succeeded

###############################################################################
# removes .o and .out files from sip/resiprocrate/test
###############################################################################
def clean(test_programs):
    for curTest in test_programs:
        outputName = curTest + ".out"
        objectName = curTest + ".o"
        os.system("rm " + outputName);
        os.system("rm " + objectName);

###############################################################################
# main
###############################################################################

# TODO: add more test files here
test_programs = ["../testCorruption.cxx",       \
                 "../testCoders.cxx",           \
                 "../testMacSecurity.cxx"       \
                ]

build_tests(test_programs)
run_tests(test_programs)
clean(test_programs)
