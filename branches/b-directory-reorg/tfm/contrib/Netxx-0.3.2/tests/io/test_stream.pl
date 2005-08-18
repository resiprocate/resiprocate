#!/usr/bin/perl

use strict;
use lib qw(../harness);
use harness;

my $client = "stream_client";
my $server = "stream_server";

if ($^O =~ /mswin/) {
    $client .= ".exe";
    $server .= ".exe";
} else {
    $client = "./$client";
    $server = "./$server";
}

my @addresses = qw(
    localhost:5005
    local://stream_socket
);

my $max_send = 4096;
my $interval = 512;

if ((stat($client))[7] <= $max_send) {
    print STDERR "$0: client binary is too small for test\n";
    exit 1;
}

my $test = harness->new("Stream IO");

foreach my $addr (@addresses) {
    for (my $i=$interval; $i<=$max_send; $i += $interval) {
	$test->start("$addr with $i bytes");

	run_server($addr, $i);
	select(undef, undef, undef, 0.75);
	my $out = `$client $addr $client $i 2>&1`;

	if ($? == 0) {
	    $test->pass;
	} else {
	    if (not defined $out or not length($out)) {
		$out = "unknown error";
	    }

	    $test->fail($out);
	}
    }
}

$test->start("Bad Address");
my $out = `$client cant_resolve_this_name.fake 1024 2>&1`;
if ($? == 0) { $test->fail("test should have triggerd return != 0"); }
else { $test->pass(); }

sub run_server {
    my ($address, $bytes) = @_;
    system("$server $address $bytes > /dev/null 2>&1 &");
}
