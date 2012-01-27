#!/usr/bin/perl
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
# test.pl (Build Test Driver for Netxx)
# Peter J Jones (pjones@pmade.org)
#
################################################################################
#
# Includes
#
################################################################################
use strict;
use Getopt::Long;
################################################################################
#
# Constants
#
################################################################################
use constant DATE		=> 'Mon Jul 23 17:38:36 2001';
use constant ID			=> '$Id: test.pl,v 1.1 2003/08/12 06:28:53 jason Exp $';
################################################################################
#
# Global Variables
#
################################################################################
my $cf_file = "test-out.cf";

my $broken_vector = <<EOT;
*** The Standard Library that comes with your C++ compiler is broken.
*** The C++ ISO Standard requires that the std::vector container keep
*** its elements in a continous buffer.
EOT

my $no_openssl_message = <<EOT;
*** Unable to link a test program with the OpenSSL library. Please
*** specify the path to the OpenSSL header files and libraries.
EOT

my @files_to_unlink;
my $use_socket_lib=0;
my %clo;

################################################################################
#
# Code Start
#
################################################################################
$|++;

GetOptions(
    \%clo,

    'debug!',

    'cxxflags',
    'enable-tls!',
    'openssl-include=s',
    'openssl-lib=s',
) or die;;

$clo{'cxxflags'}    ||= "../../tools/cxxflags";

my $exe = `$^X $clo{'cxxflags'} --exec-extension`;
chomp($exe); $exe =~ s/^\s+//; s/\s+$//;

open(CF, ">$cf_file") || die "$0: can't open $cf_file: $!\n";

####
my @vector = 0..99;
my $vector_forward = join(" ", @vector);
my $vector_reverse = join(" ", reverse @vector);
my @output;

print "    checking std::vector for continous memory ... ";
unless (compile("vector2array.cxx")) { print "failed to compile\n"; exit 1}
print "... ";

chomp(@output = `./vector2array$exe 2>&1`);
print "... ";

foreach (@output) {s/[\r\n]//g; s/^\s+//; s/\s+$//;}
if ($output[1] ne $vector_forward || $output[2] ne $vector_reverse) {
    print "test failed\n";
    print $broken_vector;
    if ($clo{'debug'}) {
	print "1: $vector_forward\n\n";
	print "2: $vector_reverse\n\n";
	print "3: $output[0]\n\n";
	print "4: $output[1]\n\n";
	print "5: $output[2]\n\n";
    }

    exit 1;
}

print "... okay\n";

#### socket libraries
print "    checking to see if we must link with a socket library ... ";
if (compile("socket.cxx", 0, $ENV{'LDFLAGS'})) {
    print "no\n";
} else {
    if (compile("socket.cxx", 0, "$ENV{'LDFLAGS'} -lsocket -lnsl")) {
	print "yes\n";
	print CF "libsocket = yes\n";
	$use_socket_lib=1;
    } else {
	print "error\n";
	print "*** I can't seem to link a socket program!\n";
	exit 1;
    }
}

#### IPv6
print "    checking to see if you have support for IPv6 ... ";
if (compile("inet6.cxx", 1)) {
    print "yes\n";
} else {
    print "no\n";
    print CF "inet6 = no\n";
}

#### openssl
if ($clo{'enable-tls'}) {
    print "    checking for TLS library (OpenSSL >= 0.9.6) ... ";
    my $extra = '';

    if ($clo{'openssl-include'}) {
	$ENV{'CXXFLAGS'} .= " -I$clo{'openssl-include'}";
    }

    if ($clo{'openssl-lib'}) {
	$extra .= " -L$clo{'openssl-lib'}";
    }

    $extra .= " -lssl -lcrypto ";
    $extra .= " -lsocket -lnsl " if $use_socket_lib;

    if (compile("openssl.cxx", 0, $extra)) {
	print "yes\n";
    } else {
	print "no\n";
	print $no_openssl_message;
	exit 1;
    }
}

sub compile {
    my $filename = shift;
    my $just_object = shift;
    my $extra = shift||'';
    my $answer;

    print "debug\n" if $clo{'debug'};

    if ($just_object) {
	print "DEBUG: $ENV{CXX} $ENV{CXXFLAGS} -c $filename\n"
	    if $clo{'debug'};

	my $out = `$ENV{CXX} $ENV{CXXFLAGS} -c $filename 2>&1`;
	$answer = !$?;
	if ($filename =~ s/(c|cxx|cc|cpp)$/o/) {
	    push @files_to_unlink, $filename;
	}

	print "DEBUG: complier said ...\n$out\n" if $clo{'debug'}
    } else {
	my $binary; ($binary = $filename) =~ s/\.(c|cxx|cc|cpp)$//;
	$binary .= $exe;
	push(@files_to_unlink, $binary);

	chomp($binary = `$^X $clo{'cxxflags'} --mkexec $binary`);
	print "DEBUG: $ENV{CXX} $ENV{CXXFLAGS} $binary $filename $extra\n"
	if $clo{'debug'};

	my $out = `$ENV{CXX} $ENV{CXXFLAGS} $binary $filename $extra 2>&1`;
	$answer = !$?;

	print "DEBUG: complier said ...\n$out\n" if $clo{'debug'}
    }

    print "   .............. final answer: " if $clo{'debug'};
    return $answer;
}

END { unlink @files_to_unlink; }
