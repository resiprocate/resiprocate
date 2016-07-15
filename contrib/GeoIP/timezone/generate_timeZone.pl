#!/usr/bin/perl
use strict;
use warnings;
use autodie;

use Text::CSV_XS ();

# Obtain timezone.txt from http://www.maxmind.com/timezone.txt

# Used to generate timeZone.c
# usage: ./generate_timeZone.pl > ../libGeoIP/timeZone.c

my $tz;

open my $fh, '<:encoding(latin1)', 'timezone.txt';

my $csv = Text::CSV_XS->new( { binary => 1, auto_diag => 1 } );

print "#include <string.h> \n";
print
    "const char* GeoIP_time_zone_by_country_and_region(const char * country,const char * region) {\n";
print "    const char* timezone = NULL;\n";
print "    if (country == NULL) {\n";
print "      return NULL;\n";
print "    }\n";
print "    if (region == NULL) {\n";
print '        region = "";', "\n";
print "    }\n";

<$fh>;    # skip first line
while ( my $row = $csv->getline($fh) ) {
    my ( $country, $region, $timezone ) = @{$row};
    die "$_ $.\n" unless $timezone;
    $tz->{$country}->{ $region || q{} } = $timezone;
}

my $first_country;

$first_country = 0;
for my $c ( sort keys %$tz ) {
    print('    if');
    $first_country ||= $c;
    my $def = delete $tz->{$c}->{q{}};
    if ( my @reg = sort keys %{ $tz->{$c} } ) {
        my @tz = map { $tz->{$c}->{$_} } @reg;

        printf( qq! ( strcmp (country, "%s") == 0 ) {\n!, $c );
        for ( 0 .. $#reg ) {

            # have regions

            print( ( $_ == 0 ) ? '        if' : '        else if' );
            printf( qq! ( strcmp (region, "%s") == 0 ) {\n!, $reg[$_] );
            printf( qq!            return "%s";\n!,                $tz[$_] );
            printf(qq!        }\n!);
        }
        if ( defined $def ) {
            printf( qq!    else { return "%s"; }\n!, $def );
        }
        else {
            print "        else {\n             return NULL;\n        }\n";
        }
        print qq[    }\n];
    }
    else {

        # only default tz
        printf( qq! ( strcmp (country, "%s") == 0 ) {\n!, $c );
        printf( qq!        return "%s";\n!,                   $def );
        printf(qq!    }\n!);

    }
}

print qq[    return timezone;\n}\n];
