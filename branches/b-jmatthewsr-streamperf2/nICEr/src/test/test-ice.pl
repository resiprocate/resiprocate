#!/usr/local/bin/perl

use Getopt::Std;

my %Options;

$ok=getopts('kn:', \%Options);

$SKIP_ERRORS = 1 if $Options{'k'};
$TEST_NAME=$Options{'n'};

$TOTAL_TESTS_FAILED=0;
$TOTAL_TESTS=0;

require 'test-config.pl';

# run as
#   test_ice.pl [-k] <config_file>
#
# Flags:
# -k -- skip errors (otherwise stop on first failure)
#
# where <config_file> contains one or more test sections with lines for
# TEST           = name of test
# PROG           = "ice_test" or "api_test" (currently)
# TYPE           = "ans-off" or "solo"
# ANS_CONFIG     = name of script to set answerer config in registry
# ANS_EXTRA_ARGS = optional string of args to give answerer
# ANS_OUT_CHECK  = script to proces output of answerer
# OFF_CONFIG     = script to set offerer config in registry
# OFF_EXTRA_ARGS = optional string of args to give offerer
# OFF_OUT_CHECK  = script to pcocess output of offerer

$FIELD_NAME           = "NAME";
$FIELD_PROG           = "PROG";
$FIELD_TYPE           = "TYPE";
$FIELD_CONFIG         = "CONFIG";
$FIELD_EXTRA_ARGS     = "EXTRA_ARGS";
$FIELD_OUT_CHECK      = "OUT_CHECK";
$FIELD_ANS_CONFIG     = "ANS_CONFIG";
$FIELD_ANS_EXTRA_ARGS = "ANS_EXTRA_ARGS";
$FIELD_ANS_OUT_CHECK  = "ANS_OUT_CHECK";
$FIELD_ANS_REMOTE     = "ANS_REMOTE_HOST";
$FIELD_OFF_CONFIG     = "OFF_CONFIG";
$FIELD_OFF_EXTRA_ARGS = "OFF_EXTRA_ARGS";
$FIELD_OFF_OUT_CHECK  = "OFF_OUT_CHECK";
$FIELD_OFF_REMOTE     = "OFF_REMOTE_HOST";
$REMOTE_DIR	      = "REMOTE_DIR";

@ALL_FIELDS = ( $FIELD_NAME, $FIELD_PROG, $FIELD_TYPE,
                $FIELD_CONFIG, $FIELD_EXTRA_ARGS, $FIELD_OUT_CHECK,
                $FIELD_ANS_CONFIG, $FIELD_ANS_EXTRA_ARGS, $FIELD_ANS_OUT_CHECK, $FIELD_ANS_REMOTE, 
                $FIELD_OFF_CONFIG, $FIELD_OFF_EXTRA_ARGS, $FIELD_OFF_OUT_CHECK , $FIELD_OFF_REMOTE, $REMOTE_DIR);

@REQ_FIELDS_ANS_OFF = ( $FIELD_NAME, $FIELD_PROG, $FIELD_TYPE,
                        $FIELD_ANS_CONFIG, $FIELD_ANS_OUT_CHECK, 
                        $FIELD_OFF_CONFIG, $FIELD_OFF_OUT_CHECK );

@REQ_FIELDS_SOLO    = ( $FIELD_NAME, $FIELD_PROG, $FIELD_TYPE,
                        $FIELD_CONFIG, $FIELD_OUT_CHECK );

@TEST_TYPE_VALS = ( "ans-off", "solo" );

if ($#ARGV != 0) {
    &Usage();
    exit(1);
}

$config_file = $ARGV[0];

if ($TEST_OUT_PREFIX =~ /\/$/) {
   # it's a directory
   if (!(-d $TEST_OUT_PREFIX)) {
     mkdir $TEST_OUT_PREFIX || die "Error creating directory $TEST_OUT_PREFIX";
   }
}

if ($TEST_NAME){
    die("TEST_OUT_PREFIX must be directory with named tests") unless -d $TEST_OUT_PREFIX;

   $TEST_OUT_PREFIX.="$TEST_NAME/";

    die("Results for test $TEST_NAME already exist") if(-d $TEST_OUT_PREFIX);
    mkdir $TEST_OUT_PREFIX || die "Error creating directory $TEST_OUT_PREFIX";
}

$didit=0;

print ".................................................................................\n";

while (%TEST_CONF = &get_test_config()) {
    $TOTAL_TESTS++;

    if ($TEST_CONF{$FIELD_TYPE} eq "ans_off") {
        $test_failed=&run_ans_off_test(%TEST_CONF);
    }
    elsif ($TEST_CONF{$FIELD_TYPE} eq "solo") {
        $test_failed=&run_solo_test(%TEST_CONF);
    }

    print ".................................................................................\n";
    
    if($test_failed){
	$TOTAL_TESTS_FAILED++;
	die("Failed") unless $SKIP_ERRORS;
    }
} 

print "\nAll tests done.\n";
print "Total  Tests: $TOTAL_TESTS\n";
print "Failed Tests: $TOTAL_TESTS_FAILED\n";

exit(0);

########################################################################################################

sub Usage {
    print "Usage: test-ice.pl [-k] <config_file>\n";
}

sub check_hdr{
    local($l,$TCONF_REF,$hdr_name,$hdr_val) = @_;

    if ($TCONF_REF->{$hdr_name}) {
        print "Bad config: processing line $l\n";
        print "$hdr_name already defined as $oldval\n";
        print "Ignoring test\n";
        return 1;
    }
    else {
        $TCONF_REF->{$hdr_name} = $hdr_val;
    }
    return 0;
}

sub get_test_config {
    if (!$conf_file_open) {
        open(CONFIG, "$config_file") || die("Couldn't open config file: $config_file");
        $conf_file_open = 1;
    }
    if ($conf_file_done) {
        return ();
    }

    %TCONF=();
    $done = 0;
 
    if ($next_test_name) {
        $TCONF{$FIELD_NAME} = $hdr_val;
    }

    while (!$done && ($l = <CONFIG>)) {
        chomp($l);
        @av_pair = split(/=/, $l);
        $hdr_name = lc($av_pair[0]);
        $hdr_val  = $av_pair[1];
        
        # remove leading and trailing whitespace
        $hdr_name =~ s/^\s+//;
        $hdr_name =~ s/\s+$//;
        $hdr_val  =~ s/^\s+//;
        $hdr_val  =~ s/\s+$//;

        if (!$hdr_name || $hdr_name =~ /^#/) {
            # ignore blank line or comment
        }
        elsif ($hdr_name eq lc($FIELD_NAME)) {
            if ($TCONF{$FIELD_NAME}) {
                $next_test_name = $hdr_val;
                $done = 1;
            }
            else {
                $TCONF{$FIELD_NAME} = $hdr_val;
            }
        }
        else {
NEXTFIELD:     foreach $field (@ALL_FIELDS) {
                if ($hdr_name eq lc($field)) {
                    if (&check_hdr($l,\%TCONF,$field,$hdr_val)) {
                        return ();
                    }
                    last NEXTFIELD;
                }
            }
        }

    }
    if (!$l) {
        $conf_file_done = 1;
    } 

SWITCH: for (lc($TCONF{$FIELD_TYPE})) {
        /ans_off/ && do { @CHECK_FIELDS = @REQ_FIELDS_ANS_OFF ; last; };
        /solo/    && do { @CHECK_FIELDS = @REQ_FIELDS_SOLO ; last; };
    # default:
        print "Missing field: $FIELD_TYPE (\"ans_off\" or \"solo\")\n";
        print "Missing info in test config.  Ignoring test '$TCONF{$FIELD_NAME}'.\n";
        return ();
    }

if (0) {
    print "Config:\n";
    foreach $field (@ALL_FIELDS) {
       if ($TCONF{$field}) {
           print "  $field = $TCONF{$field}\n";
       }
    }
}

    foreach $field (@CHECK_FIELDS) {
        if (!$TCONF{$field}) {
            print "Missing field: $field\n";
            print "Missing info in test config.  Ignoring test.\n";
            return ();
        }
    }

    $TCONF{$FIELD_TYPE} = lc($TCONF{$FIELD_TYPE});

    %TCONF;
}

sub run_solo_test {
    local(%TCONF) = @_;

    $failed = 0;

    $out_prefix = $TEST_OUT_PREFIX . $TCONF{$FIELD_NAME};
    $out_prefix =~ s/\s/_/g;
    $out = $out_prefix . ".out";

    print "--------------------------------------------\n";
    print "Running test: $TCONF{$FIELD_NAME}\n";
    print "--------------------------------------------\n";

    eval {
        local $SIG{ALRM} = sub { die "timeout\n" };
        alarm $PROGRAM_TIMEOUT;

        system("killall $TCONF{$FIELD_PROG} > /dev/null 2>&1");

        # run registry
        system("killall registryd > /dev/null 2>&1");
        system("$REGISTRYD > /dev/null 2>&1 &") == 0 or 
            die("Error running registryd: $REGISTRYD");

        # load answerer config
        $prog = $TCONF{$FIELD_CONFIG};
        #system("$prog > /dev/null 2>&1") == 0 or
        system("$prog > /dev/null 2>&1") == 0 or
            die("Error setting config: $prog");
    
        unlink($out);

        print "Output saved to: $out\n";

        # run solo program
        $prog = $ICE_BINDIR . $TCONF{$FIELD_PROG} . " -R " . $TCONF{$FIELD_EXTRA_ARGS};
        if(system("$prog > $out 2>&1") != 0){
	    print "\nERROR running Test $TCONF{$FIELD_NAME} -- Test FAILED.\n\n";
	    $failed = 1;
	}
	
        alarm 0;
    };
    if ($@) {
        die unless $@ eq "timeout\n";
        print "\nTIMEOUT running Test $TCONF{$FIELD_NAME} -- Test FAILED.\n\n";
        $failed = 1;
    }

    # grep through output file for particular things

    system("killall $TCONF{$FIELD_PROG} > /dev/null 2>&1");
    system("killall registryd > /dev/null 2>&1");

    if ($failed) {
        return 1;
    }

    eval {
        local $SIG{ALRM} = sub { die "timeout\n" };
        alarm $OUT_CHECK_TIMEOUT;

        $prog = $TCONF{$FIELD_OUT_CHECK};
        $r = system("$prog $out");
        if ($r) {
            print "Answerer output checker ERROR: \'$prog $out\' returned error.\n";
        }
        alarm 0;
    };
    if ($@) {
        die unless $@ eq "timeout\n";
        print "\nTIMEOUT on output checker for Test $TCONF{$FIELD_NAME} -- Test FAILED.\n\n";
        return 1;
    }

    if ($r) {
        print "\nTest $TCONF{$FIELD_NAME} FAILED.\n\n";
	return 1;
    }
    else {
        print "\nTest $TCONF{$FIELD_NAME} PASSED.\n\n";
	return 0;
    }
    #unlink($out);

}

sub run_ans_off_test {
    local(%TCONF) = @_;
    local($same_host)=0;
    local($off_host)=$TCONF{$FIELD_OFF_REMOTE};
    local($ans_host)=$TCONF{$FIELD_ANS_REMOTE};

    $same_host = 1 if $off_host eq $ans_host;

    $failed = 0;

    $out_prefix = $TEST_OUT_PREFIX . $TCONF{$FIELD_NAME};
    $out_prefix =~ s/\s/_/g;
    $ans_out = $out_prefix . "_ans.out";
    $off_out = $out_prefix . "_off.out";

    print "--------------------------------------------\n";
    print "Running test: $TCONF{$FIELD_NAME}\n";
    print "--------------------------------------------\n";

    eval {
        local $SIG{ALRM} = sub { die "timeout\n" };
        alarm $PROGRAM_TIMEOUT;

        &execute($off_host,"killall $TCONF{$FIELD_PROG} > /dev/null 2>&1");
        &execute($ans_host,"killall $TCONF{$FIELD_PROG} > /dev/null 2>&1");

        # run registry
        &execute($off_host,"killall registryd > /dev/null 2>&1");
        &execute($ans_host,"killall registryd > /dev/null 2>&1");
        &execute($off_host,"$REGISTRYD > /dev/null 2>&1 &") == 0 or 
            die("Error running registryd: $REGISTRYD");

	if(!$same_host){
	    &execute($ans_host,"$REGISTRYD > /dev/null 2>&1 &") == 0 or 
		die("Error running registryd: $REGISTRYD");
	}

        # load answerer config
        $prog = $TCONF{$FIELD_ANS_CONFIG};
        #system("$prog > /dev/null 2>&1") == 0 or
        &execute($ans_host,"$prog > /dev/null 2>&1") == 0 or
            die("Error setting ans config: $prog");
    
        unlink($ans_out);
        unlink($off_out);
        print "Answerer output saved to: $ans_out\n";
    
        # run answerer >& ans_out
        $prog = $ICE_BINDIR . $TCONF{$FIELD_PROG} . " -a -R " . $TCONF{$FIELD_ANS_EXTRA_ARGS};
        &execute($ans_host,"$prog",$ans_out,1) == 0 or
            die("Error running answerer: $prog");

        # wait a few secs for the program to load its config from the registry
        sleep(2);
    
        # load offerer config
        $prog = $TCONF{$FIELD_OFF_CONFIG};
        &execute($off_host,"$prog > /dev/null 2>&1") == 0 or
            die("Error setting off config: $prog");

        print "Offerer output saved to: $off_out\n\n";

        # run offerer >& off_out
        $prog = $ICE_BINDIR . $TCONF{$FIELD_PROG} . " -o -R " . $TCONF{$FIELD_OFF_EXTRA_ARGS};

        if(&execute($off_host,"$prog",$off_out,0) != 0){
	    print "\nERROR running Test $TCONF{$FIELD_NAME} -- Test FAILED.\n\n";
	    $failed = 1;
	}

        alarm 0;
    };
    if ($@) {
        die unless $@ eq "timeout\n";
        print "\nTIMEOUT running Test $TCONF{$FIELD_NAME} -- Test FAILED.\n\n";
        $failed = 1;
    }

    # grep through output files for particular things

    &execute($off_host,"killall $TCONF{$FIELD_PROG} > /dev/null 2>&1");
    &execute($ans_host,"killall $TCONF{$FIELD_PROG} > /dev/null 2>&1");
    &execute($off_host,"killall registryd > /dev/null 2>&1");
    &execute($ans_host,"killall registryd > /dev/null 2>&1");
    
    if ($failed) {
        return 1;
    }

    eval {
        local $SIG{ALRM} = sub { die "timeout\n" };
        alarm $OUT_CHECK_TIMEOUT;

        $prog = $TCONF{$FIELD_ANS_OUT_CHECK};
        $rA = system("$prog $ans_out");
        if ($rA) {
            print "Answerer output checker ERROR: \'$prog $ans_out\' returned error.\n";
        }
        alarm 0;
    };
    if ($@) {
        die unless $@ eq "timeout\n";
        print "\nTIMEOUT on Answerer output checker for Test $TCONF{$FIELD_NAME} -- Test FAILED.\n\n";
        return 1;
    }

    eval {
        local $SIG{ALRM} = sub { die "timeout\n" };
        alarm $OUT_CHECK_TIMEOUT;

        $prog = $TCONF{$FIELD_OFF_OUT_CHECK};
        $rO= system("$prog $off_out");
        if ($rO) {
            print "Offerer output checker ERROR: \'$prog $off_out\' returned error.\n";
        }
    };
    if ($@) {
        die unless $@ eq "timeout\n";
        print "\nTIMEOUT on Offerer output checker for Test $TCONF{$FIELD_NAME} -- Test FAILED.\n\n";
        return 1;
    }


    if ($rA || $rO) {
        print "\nTest $TCONF{$FIELD_NAME} FAILED.\n\n";
	return 1;
    }
    else {
        print "\nTest $TCONF{$FIELD_NAME} PASSED.\n\n";
	return 0;
    }
    #unlink($ans_out);
    #unlink($off_out);
}

sub execute {
    local($remote_host,$command,$out_file,$bg)=@_;

    if($remote_host){
	$ncomm="ssh $remote_host '(cd $TCONF{$REMOTE_DIR};". $command . ")'";
	$ncomm.="> $out_file 2>&1 " if $out_file;
	$ncomm.="&" if $bg;
	
	print STDERR "$ncomm\n";
	system($ncomm);
    }
    else{
	$command.="> $out_file 2>&1 " if $out_file;
	$command.="&" if $bg;

	system($command);
    }
}
