#!/usr/local/bin/perl

$DROPROOT="/home/pcecap/dist";
#$DROPROOT="/usr/home/acain/dist_ice/dest";
$MAKE="make";
#$MAKE="gmake";

$version="1.0-fcs2";
$date=`date +%Y%m%d`;
chop $date;
$filename="ice-$version-$date";

$CRUFT="*.[chod]";

die("Couldn't find drop directory") unless -d $DROPROOT;

$DROPDIR="$DROPROOT/ice-$version";

system("rm -rf $DROPDIR; mkdir $DROPDIR");
system("tar -cf - . | (cd $DROPDIR; tar xpf -)");

print "Cleaning out build dirs...\n";
system("cd $DROPDIR/make/darwin/; $MAKE clean; rm -f $CRUFT");
system("cd $DROPDIR/make/linux-fedora/; $MAKE clean; rm -f $CRUFT");

print "Removing cruft...\n";
system("cd $DROPDIR; find . -name '.cvsignore' -exec rm {} \\; -print");
system("cd $DROPDIR; find . -name '.scripts_munged' -exec rm {} \\; -print");
system("cd $DROPDIR; find . -name '*~' -exec rm {} \\; -print");
system("cd $DROPDIR; find . -name '*.core' -exec rm {} \\; -print");
system("cd $DROPDIR; find . -name '*.tr' -exec rm {} \\; -print");
system("cd $DROPDIR; find . -name '.#*' -exec rm {} \\; -print");

print "Removing unwanted files/dirs...\n";
system("rm -rf $DROPDIR/util/scripts/");
system("rm -rf $DROPDIR/stun/*.pem");
system("rm -rf $DROPDIR/stun/TODO");
system("rm -rf $DROPDIR/stun/README.txt");
system("rm -rf $DROPDIR/stun/draft-ietf-behave-rfc3489bis-06.txt");
system("rm -rf $DROPDIR/stun/rfc3489.txt");
system("rm -rf $DROPDIR/stun/stund.xcode/");
system("rm -rf $DROPDIR/stun/WinStun/");
system("rm -rf $DROPDIR/stun/WinStunSetup/");
system("rm -rf $DROPDIR/stun/client.c");
system("rm -rf $DROPDIR/stun/server.c");
system("rm -rf $DROPDIR/stun/tlsServer.c");
system("rm -rf $DROPDIR/testua/");
system("rm -rf $DROPDIR/async_call/main.c");
system("rm -rf $DROPDIR/async_call/main.cxx");
system("rm -rf $DROPDIR/make/freebsd/");
system("rm -rf $DROPDIR/async_call/Makefile");
system("rm -rf $DROPDIR/api/Makefile");

print "Adjusting build-related dirs...\n";
&replace_openssl_pcap_paths("$DROPDIR/make/linux-fedora/Makefile");
# leave darwin Makefile alone:
# &replace_openssl_pcap_paths("$DROPDIR/make/darwin/Makefile");

print "Removing test dirs...\n";
system("rm -rf $DROPDIR/stun/test");
system("rm -rf $DROPDIR/util/test");
system("rm -rf $DROPDIR/net/test");
system("rm -rf $DROPDIR/async/test");
system("mv $DROPDIR/test/test-results.txt $DROPDIR");
system("rm -rf $DROPDIR/test");

print  "Removing CVS dirs...\n";
#system("cd $DROPDIR; find . -type d -name 'CVS' -exec rm -rf {} \\;");
system("cd $DROPDIR; rm -rf `find . -type d -name 'CVS' -print`");

system("rm -f $DROPDIR/$filename.tar.gz");
system("cd $DROPROOT; tar cf - ice-$version | gzip > $DROPROOT/$filename.tar.gz");

sub replace_openssl_pcap_paths {
  local($infile) = @_;
  open (INFILE, "<$infile");
  open (OUTFILE, ">$infile.tmp");
  while ($line = <INFILE>)  {
     if($line =~ /OPENSSL_SRC_DIR\s*=/){
       $line = "OPENSSL_SRC_DIR=../../../openssl-0.9.7l/\n";
     }
     if($line =~ /LIBEDIT_SRC_DIR\s*=/){
       $line = "LIBEDIT_SRC_DIR=../../../libedit-20060829-2.9/\n";
     }
     print OUTFILE $line;
  }
  close (INFILE);
  close (OUTFILE);
  unlink $infile;
  rename("$infile.tmp", "$infile") || die "rename failed: $!";
}
