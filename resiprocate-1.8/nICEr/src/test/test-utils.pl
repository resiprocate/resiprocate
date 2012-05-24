#!/usr/bin/perl

require 'test-config.pl';

sub NR_reg_set {
  print("$NRREGCTL write @_\n");
  system("$NRREGCTL write @_") == 0 or die ("error running $NRREGCTL");
}

sub NR_reg_set_char {
  local($kval) = $_[1];

  if(!($kval =~ '0x') && !($kval =~ '\\\\')) {
    $kval = sprintf("0x%x", $kval);
  }
  return &NR_reg_set("char", $_[0], $kval);
}

sub NR_reg_set_uchar {
  local($kval) = $_[1];

  if(!($kval =~ '0x') && !($kval =~ '\\\\')) {
    $kval = sprintf("0x%x", $kval);
  }
  return &NR_reg_set("UCHAR", $_[0], $kval);
}

sub NR_reg_set_string {
  return &NR_reg_set("string",@_);
}

sub NR_reg_set_uint2 {
  return &NR_reg_set("uint2",@_);
}

sub NR_reg_set_uint4 {
  return &NR_reg_set("uint4",@_);
}

1;

