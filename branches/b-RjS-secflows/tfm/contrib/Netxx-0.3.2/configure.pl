######################################################################
# Copyright (C) 2001-2003 Peter J Jones (pjones@pmade.org)
# All Rights Reserved
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 3. Neither the name of the Author nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR
# OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
# USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
# OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
################################################################################
#
# configure.pl (Configure the Netxx library)
# Peter J Jones (pjones@pmade.org)
#
################################################################################
#
# Includes
#
################################################################################
use strict;
use Cwd qw(cwd chdir);
use Getopt::Long;
################################################################################
#
# Constants
#
################################################################################
use constant DATE		=> 'Mon Sep  3 14:38:06 2001';
use constant ID			=> '$Id: configure.pl,v 1.1 2003/08/12 06:28:53 jason Exp $';
################################################################################
#
# Global Variables
#
################################################################################
my $libname = "Netxx";
my $pwd = cwd;

my $mkmf	= "$pwd/tools/mkmf";
my $cxxflags	= "$pwd/tools/cxxflags";
my $genconfig	= "$pwd/tools/genconfig";

my $mkmf_flags = "--cxxflags='$cxxflags' --quiet";
my $genconfig_flags = '';

my $install_spec = "$pwd/docs/install.spec";
my $version_file = "$pwd/docs/VERSION";
my $lib_version;
my $lib_major;

my @linkwith;
my ($openssl_include, $openssl_lib);
my $build_test_dir	= "tests/build";
my $build_test		= "test.pl";
my $build_test_conf	= "tests/build/test-out.cf";
my %test_data;

my @base_source_files = qw(
    Accept.cxx
    Address.cxx
    Datagram.cxx
    DatagramServer.cxx
    OSError.cxx
    Peer.cxx
    Probe.cxx
    Probe_select.cxx
    RecvFrom.cxx
    Resolve_gethostbyname.cxx
    Resolve_getservbyname.cxx
    ServerBase.cxx
    SockAddr.cxx
    Socket.cxx
    SockOpt.cxx
    StreamBase.cxx
    Stream.cxx
    StreamServer.cxx
);

my @tls_source_files = qw(
    TLS_Certificate.cxx
    TLS_Context.cxx
    TLS_Stream.cxx
    TLS_tls_pimpl.cxx
);

my @example_files = qw(
    http_get.cxx
    multi_server.cxx
    streambuf_echo_client.cxx
    tcp_daytime_client.cxx
    tcp_echo_client.cxx
    tcp_echo_server.cxx
    udp_daytime_client.cxx
    udp_echo_client.cxx
    udp_echo_server.cxx
);


my @tls_example_files = qw(
    http_https_server.cxx
    https_get.cxx
    tls_echo_client.cxx
    tls_echo_server.cxx
);

my $inet6_code = "inet6.cxx";
my %clo;
################################################################################
#
# Code Start
#
################################################################################
$|++;

print <<EOT;
      
      888b      88
      8888b     88              ,d
      88 `8b    88              88
      88  `8b   88  ,adPPYba, MM88MMM 8b,     ,d8 8b,     ,d8
      88   `8b  88 a8P_____88   88     `Y8, ,8P'   `Y8, ,8P'
      88    `8b 88 8PP"""""""   88       )888(       )888(
      88     `8888 "8b,   ,aa   88,    ,d8" "8b,   ,d8" "8b,
      88      `888  `"Ybbd8"'   "Y888 8P'     `Y8 8P'     `Y8
      
      ooooooooooooooooooooooooooooooooooooooooooooooooooooooo
      
EOT

if (not defined $ENV{'CXX'}) {
    print STDERR "**** your CXX environment variable is not set. Netxx needs this    ****\n";
    print STDERR "**** variable to find your C++ compiler. Please set it to the path ****\n";
    print STDERR "**** to your compiler and re-run configure.pl. For now, I will     ****\n";
    print STDERR "**** just use c++ as your compiler.                                ****\n";
}

$ENV{'CXXFLAGS'}    ||= '';
$ENV{'CXX'}	    ||= 'c++';

GetOptions(
    \%clo,

    'help|h',
    'developer!',
    'contrib!',
    'debug!',
    'disable-examples!',
    'disable-ipv6!',
    'disable-streambuf!',
    'disable-shared!',

    'enable-tls!',
    'openssl-prefix=s',
    'openssl-include=s',
    'openssl-lib=s',

    'prefix=s',
    'libdir=s',
    'incdir=s',
    'bindir=s',
) or usage();
$clo{'help'} && usage();

sub usage {
    print "Usage: $0 [options]\n", <<EOT;
  --help, h              This message
  --debug                See what this script is doing
  --developer            Put library into Netxx developer mode
  --contrib              Build Netxx as part of a larger project
  --disable-examples     Don't generate Makefile for examples
  --disable-ipv6         Don't use IPv6 at all.
  --disable-streambuf    Don't support the use of the streambuf code
  --disable-shared       Don't build a shared library

  --enable-tls           Compile in TLS support classes
  --openssl-prefix path  Path to OpenSSL base path (ie /usr)
  --openssl-include path Specific path to OpenSSL include files
  --openssl-lib path     Specific path to OpenSSL libraries

  --prefix path          Set install prefix to path [/usr/local]
  --libdir path          Set library dir to path [PREFIX/lib]
  --incdir path          Set include dir to path [PREFIX/include]
  --bindir path          Set bin dir to path [PREFIX/bin]
EOT
    exit;
}

$clo{'prefix'}	    ||= '/usr/local';
$clo{'libdir'}	    ||= "$clo{'prefix'}/lib";
$clo{'incdir'}	    ||= "$clo{'prefix'}/include";
$clo{'bindir'}	    ||= "$clo{'prefix'}/bin";

my $source_files = join(' ', sort @base_source_files);
$mkmf_flags .= " --include $pwd/include";

if ($clo{'developer'}) {
    $mkmf_flags .= " --developer";
}

setup_openssl();
run_build_tests();

print "Creating Makefiles ... ";
get_version();
gen_src_makefile();
gen_examples_makefile();
gen_top_makefile();
print "done\n";

generate_config_script();

if (!$clo{'contrib'}) {
    my $support_ipv6;
    
    if ((exists $test_data{'inet6'} && $test_data{'inet6'} eq 'no') || $clo{'disable-ipv6'}) {
	$support_ipv6 = 'no';
    } else {
	$support_ipv6 = 'yes';
    }

    print "\n";
    print "*********************************** Netxx $lib_version Configuration\n";
    print " Shared Library Support: ", ($clo{'disable-shared'} ? "no\n" : "yes\n");
    print "  Netxx::Netbuf Support: ", ($clo{'disable-streambuf'} ? "no\n" : "yes\n");
    print "    Native IPv6 Support: $support_ipv6\n";
    print "     Netxx::TLS Support: ", ($clo{'enable-tls'} ? "yes\n" : "no\n");
    print "         Build Examples: ", ($clo{'disable-examples'} ? "no\n" : "yes\n");
    print "      Developer Support: yes\n" if $clo{'developer'};
    print "        Put binaries in: $clo{'bindir'}\n";
    print "       Put libraries in: $clo{'libdir'}\n";
    print "    Put header files in: $clo{'incdir'}\n";
    print "\n";
}

if (!$clo{'contrib'}) {
    print "+-----------------------------------------------------------+\n";
    print "|      Please join the Netxx Announcement mailing list      |\n";
    print "|   http://pmade.org/pjones/software/netxx/resources.html   |\n";
    print "+-----------------------------------------------------------+\n";
}
######################################################################
sub get_version {
    unless (open(VER, $version_file)) {
	print "\n"; print STDERR "$0: can't open $version_file: $!\n";
	exit 1;
    }

    my $first_line = <VER>;
    close VER;

    my @fields = split(/\s+/, $first_line);
    $lib_version = $fields[0];
    $lib_major   = $fields[1];
}
######################################################################
sub gen_src_makefile {
    unless (chdir('src')) {
	print "\n"; print STDERR "$0: can't chdir to src: $!\n";
	exit 1;
    }

    my $extra_flags = "--static-lib $libname ";
    $extra_flags .= "--shared-lib $libname --major $lib_major " unless $clo{'disable-shared'};

    if ($clo{'enable-tls'} && $openssl_include) {
	$extra_flags .= "--include $openssl_include ";
    }

    system("$^X $mkmf $mkmf_flags $extra_flags $source_files");
    chdir($pwd);

    print "... ";
}
######################################################################
sub gen_examples_makefile {
    return if $clo{'disable-examples'};

    unless (chdir("examples")) {
	print "\n"; print STDERR "$0: can't chdir to examples: $!\n";
	exit 1;
    }

    my $extra_flags = "--slinkwith '$pwd/src,$libname' ";
    foreach (@linkwith) {
	if (ref $_) {
	    $extra_flags .= "--linkwith '$_->[0],$_->[1]' ";
	} else {
	    $extra_flags .= "--linkwith '$_' ";
	}
    }

    if ($clo{'disable-streambuf'}) {
	@example_files	    = grep {$_ ne 'streambuf_echo_client.cxx'} @example_files;
	@tls_example_files  = grep {$_ ne 'http_https_server.cxx'}     @tls_example_files;
    }

    my $files = join(' ', @example_files);
    $files .= ' ' . join(' ', @tls_example_files) if $clo{'enable-tls'};

    system("$^X $mkmf $mkmf_flags $extra_flags --many-exec $files");
    chdir($pwd);

    print "... ";
}
######################################################################
sub gen_top_makefile {
    unless (open(SPEC, ">$install_spec")) {
	print "\n"; print STDERR "$0: can't create $install_spec: $!\n";
	exit 1;
    }

    print SPEC "LIBDIR=$clo{'libdir'}\n";
    print SPEC "INCLUDEDIR=$clo{'incdir'}\n";
    print SPEC "BINDIR=$clo{'bindir'}\n";

    print SPEC "binary $libname-config\n";
    print SPEC "include-dir $pwd/include/$libname $libname\n";
    print SPEC "static-lib $pwd/src $libname\n";
    print SPEC "shared-lib $pwd/src $libname $lib_major\n" unless $clo{'disable-shared'};

    close SPEC;

    my @dirs = ('src');
    push(@dirs, 'examples') unless ($clo{'disable-examples'});

    system("$^X $mkmf $mkmf_flags --install $install_spec --wrapper @dirs");
    unlink($install_spec);

    print "... ";
}
######################################################################
sub run_build_tests {
    my $extra = $clo{'debug'} ? '--debug' : '';

    print "Testing build enviornment ...\n";
    chdir($build_test_dir) || die "$0: can't chdir to $build_test_dir: $!";

    exit 1 if system("$^X $build_test $extra");
    chdir($pwd);

    open(CF, $build_test_conf) or die "$0: can't open test output: $!\n";
    while (<CF>) {
	chomp;
	my ($key, $value) = split(/\s*=\s*/, $_, 2);
	$test_data{$key} = $value;
    }
    close(CF);

    if ((exists $test_data{'inet6'} && $test_data{'inet6'} eq 'no') || $clo{'disable-ipv6'}) {
	$source_files .= " $inet6_code";
	$ENV{'CXXFLAGS'} .= " -DNETXX_NO_INET6";
    }

    if (exists $test_data{'libsocket'} and $test_data{'libsocket'} eq 'yes') {
	push(@linkwith, "socket", "nsl");
    }

    unlink($build_test_conf);
}
######################################################################
sub setup_openssl {
    if ($clo{'enable-tls'}) {
	$build_test .= " --enable-tls";

	if ($clo{'openssl-prefix'}) {
	    $openssl_include = "$clo{'openssl-prefix'}/include";
	    $openssl_lib     = "$clo{'openssl-prefix'}/lib";
	}

	if ($clo{'openssl-include'}) {
	    $openssl_include = $clo{'openssl-include'};
	}

	if ($clo{'openssl-lib'}) {
	    $openssl_lib = $clo{'openssl-lib'};
	}

	if ($openssl_include) {
	    $build_test .= " --openssl-include $openssl_include";
	}

	if ($openssl_lib) {
	    $build_test .= " --openssl-lib $openssl_lib";
	    push(@linkwith, "$openssl_lib,ssl", "$openssl_lib,crypto");
	} else {
	    push(@linkwith, "ssl", "crypto");
	}

	$source_files .= ' ' . join(' ', @tls_source_files);
    }

}
################################################################################
sub generate_config_script {
    my ($all_incs, $all_libs);
    my $version = $lib_version;

    if ($clo{'enable-tls'}) {
	$version .= "-TLS";
    }

    if ($clo{'contrib'}) {
	$all_incs = "--include $pwd/include ";
	$all_libs = "--slinkwith $pwd/src,$libname ";
	foreach (@linkwith) { $all_libs .= "--linkwith $_ "; }

    } else {
	$all_incs = "-I$clo{'incdir'} ";
	$all_libs = `$^X $cxxflags --linkwith $clo{'libdir'},$libname`;
	foreach (@linkwith) { $all_libs .= `$^X $cxxflags --linkwith $_`; }
    }

    # clean and encode
    foreach ($all_incs, $all_libs) {
	s/^\s+//; s/\s+$//; s/\s+/ /g; s/-/^/g;
    }

    my $command  = "$^X $genconfig --version $version --name $libname";
       $command .= " --libs \"$all_libs\" --cxxflags \"$all_incs\"";
       $command .= " -o $libname-config";

    print "Creating $libname-config script ... ";
    $_ = `$command 2>&1`;
    print "done\n";
}
