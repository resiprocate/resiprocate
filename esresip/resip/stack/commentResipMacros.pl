#!/usr/bin/perl

open(FILE,$ARGV[0]);
my @arrayThingy = <FILE>;
my $string = join('',@arrayThingy);
close(FILE);
$_=$string;


if($ARGV[0] =~ m/SipMessage\.hxx/)
{
   s/(^[ ]*defineHeader\([^,]*,\s*\"([^"]*)\"\s*,[^,]*,\s*\"([^)]*)\"\s*\);)/\/** \@ brief Accessor for the \2 header, (\3). *\/ \n\1/mgs;
   s/(^[ ]*defineMultiHeader\([^,]*,\s*\"([^"]*)\"\s*,[^,]*,\s*\"([^)]*)\"\s*\);)/\/** \@brief Accessor for the \2 header (\3). *\/ \n\1/mgs;
}

if($ARGV[0] =~ m/Headers\.hxx/)
{
   s/(^[ ]*defineHeader\(([^,]*),\s*\"([^"]*)\"\s*,[^,]*,\s*\"([^)]*)\"\s*\);)/\/** \@brief Accessor token for the \3 header (\4). \n \@note There is a static instance of this named h_\2. *\/ \n\1/mgs;
   s/(^[ ]*defineMultiHeader\(([^,]*),\s*\"([^"]*)\"\s*,[^,]*,\s*\"([^)]*)\"\s*\);)/\/** \n \@brief Accessor token for the \3 header (\4). \n \@note There is a static instance of this named h_\2s. *\/ \n\1/mgs;
}

if($ARGV[0] =~ m/ParserCategory\.hxx/)
{
   s/(^[ ]*defineParam\(([^,]*),\s*\"([^"]*)\"\s*,[^,]*,\s*\"([^)]*)\"\s*\);)/\/** \@brief Accessor for the \3 parameter (\4). *\/ \n\1/mgs;
}

if($ARGV[0] =~ m/ParameterTypes\.hxx/)
{
   s/(^[ ]*defineParam\(([^,]*),\s*\"([^"]*)\"\s*,[^,]*,\s*\"([^)]*)\"\s*\);)/\/** \@brief Accessor token for the \3 parameter (\4). \n \@note There is a static instance of this named p_\2. *\/ \n\1/mgs;
}

print $_;
