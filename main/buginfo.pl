#!/usr/bin/perl

$uname = `uname -a`;

&section ("uname -a");
print "$uname \n";

$cc = `make show.CC`;
chop $cc;
$cc =~ s/.*=//g;

&exec("$cc --version");

$conf = `grep := build/Makefile.conf`;
&section("Makefile.conf");
print $conf."\n";

while ($conf =~ /([^ ]*) *:= *(.*)/g)
{
  $name = $1;
  $val = $2;
  $name =~ s/[\r\n]//g;
  $val =~ s/[\r\n]//g;
  $conf{$name} = $val;
}

if ($conf{'USE_SSL'} eq 'yes')
{
  if (length ($conf{SSL_LOCATION}))
  {
    &exec("${SSL_LOCATION}/apps/openssl version");
  }
  else
  {
    &exec("openssl version");
  }
}

&exec ("svnversion");

if (-e 'ReleaseNotes.txt')
{
  &exec ("head ReleaseNotes.txt");
}

if ($uname =~ /Darwin/i)
{
  &exec ("sysctl -a hw");
}
elsif ($uname =~ /Linux/i)
{
  &exec ("cat /proc/cpuinfo");
}


sub exec
{
  my ($cmd) = shift;
  &section($cmd);
  print `$cmd`."\n";
}

sub section
{
  my ($title) = shift;
  my ($center) = int(36 - length($title)/2);
  print (('='x$center)." $title ".('='x$center)."\n");
}
