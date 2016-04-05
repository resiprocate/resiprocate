#!/usr/bin/perl

# Used to generate regionName.c
# usage: ./generate_regionName.pl > ../libGeoIP/regionName.c

# Usually run update-region-codes.pl before generate_regionName.pl like
#
# ./update-region-codes.pl
# ./generate_regionName.pl > ../libGeoIP/regionName.c

use strict;
use warnings;
use autodie;

use Text::CSV_XS ();

my $last_country_code = q{};

my @country_codes;
my @region_code2s;
my @region_names;

sub init_variables () {
    $last_country_code = q{};
}

sub parse_file {

    # http://geolite.maxmind.com/download/geoip/misc/region_codes.csv
    open my $fh, '<:encoding(latin1)', 'region_codes.csv';
    <$fh>;

    my $csv = Text::CSV_XS->new( { binary => 1, auto_diag => 1 } );

    while ( my $row = $csv->getline($fh) ) {
        my ( $country_code, $region_code, $name ) = @{$row};
        my $region_code2;
        if ( $country_code eq 'US' || $country_code eq 'CA' ) {
            $region_code =~ /^[A-Z]{2}$/ or die "Wrong region code";
            $region_code2
                = ( ( ord( substr( $region_code, 0, 1 ) ) - 48 )
                * ( 65 + 26 - 48 ) )
                + ord( substr( $region_code, 1, 1 ) )
                - 48 + 100;
        }
        else {
            if ( $region_code =~ /^\d\d$/ ) {
                $region_code2
                    = ( ord( substr( $region_code, 0, 1 ) ) - 48 ) * 10
                    + ord( substr( $region_code, 1, 1 ) )
                    - 48;

            }
            elsif ( $region_code =~ /^[A-Z0-9]{2}$/ ) {
                $region_code2
                    = ( ( ord( substr( $region_code, 0, 1 ) ) - 48 )
                    * ( 65 + 26 - 48 ) )
                    + ord( substr( $region_code, 1, 1 ) )
                    - 48 + 100;
            }
            else {
                die "Region code seems wrong $region_code\n";
            }
        }
        die q{region name cannot contain a quote (")} if $name =~ /"/;
        readcode( $country_code, $region_code, $region_code2, $name );
    }
    close $fh;
}

sub generate_region_name_functions {

    my $i = 0;
    for my $country_code (@country_codes) {

        # function head
        if ( $last_country_code ne $country_code ) {
            if ( $last_country_code ne q{} ) {
                print "    default:\n        return NULL;\n";
                print "    }\n";    # switch end
                print "}\n";             # function get_region_name_XX end
            }
            $last_country_code = $country_code;

            print "\nstatic const char * get_region_name_" . $country_code
                . "(int region_code)\n";    # function begin
            print "{\n";
            print "    switch(region_code) {\n";    # switch end
        }

        print "    case " . $region_code2s[$i] . ":\n";
        print qq{        return "$region_names[$i]";\n};
        $i++;
    }

    if ( $last_country_code ne "" ) {
        print "    default:\n        return NULL;\n";
        print "    }\n";    # switch end
        print "}\n";             # function get_region_name_XX end
    }
}

# read code to collect to array
sub readcode {
    my ( $country_code, $region_code, $region_code2, $name ) = @_;

    push( @country_codes, $country_code );
    push( @region_code2s, $region_code2 );
    push( @region_names,  $name );

}

sub generate_region_condition () {

    for my $country_code (@country_codes) {
        if ( $last_country_code ne $country_code ) {
            $last_country_code = $country_code;

            print "    if (strcmp(country_code," . qq(")
                . $last_country_code . qq(")
                . ") == 0) {\n";    # if condition
            print "        return get_region_name_"
                . $last_country_code
                . "(region_code2);\n";
            print "    }\n";    # function get_region_name_XX end
        }
    }
}

# implementation of GeoIP_region_name_by_code
sub generate_api_function {

    print <<__C_CODE__;

const char * GeoIP_region_name_by_code(const char * country_code,const char * region_code) {
    int region_code2 = -1;
    if (region_code == NULL) { return NULL; }

    if (   ((region_code[0] >= 48) && (region_code[0] < (48 + 10)))
        && ((region_code[1] >= 48) && (region_code[1] < (48 + 10)))
    ) {

        /* only numbers, that shortens the large switch statements */
        region_code2 = (region_code[0] - 48) * 10 + region_code[1] - 48;
    }

    else if (    (    ((region_code[0] >= 65) && (region_code[0] < (65 + 26)))
                   || ((region_code[0] >= 48) && (region_code[0] < (48 + 10))))
              && (    ((region_code[1] >= 65) && (region_code[1] < (65 + 26)))
                   || ((region_code[1] >= 48) && (region_code[1] < (48 + 10))))
    ) {

        region_code2 = (region_code[0] - 48) * (65 + 26 - 48) + region_code[1] - 48 + 100;
    }

    if (region_code2 == -1) {return NULL;}

__C_CODE__

    init_variables();
    generate_region_condition();

    # function end
    print "    return NULL;\n";
    print "}\n";

}

parse_file();
init_variables();
print qq{#include "GeoIP.h"\n};
print "#include <string.h>\n";
print "#include <stdio.h>\n\n";
generate_region_name_functions();
generate_api_function();

