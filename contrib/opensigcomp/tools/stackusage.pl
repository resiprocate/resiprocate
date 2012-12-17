#!/usr/bin/perl

# This script parses the result of running "src/speed-test 1"
# with stack debugging turned on. It spits out the call chain
# that results in the largest stack usage. It's very rudimentary,
# but enough to extract information.

# Replace "base" with the location and size of the stack usage
# for main().
$base = 0xbfd9be88-16996;

while (<DATA>)
{
  chop;
  if (/^\@(0x[0-9a-f]{8})\+([0-9]+) [^ ]+ (.*)/)
  {
    $dump = 0;
    $stackSize = $base - eval($1);
    $stackSize += $2;
    if ($stackSize > $maxStackSize)
    {
      $maxStackSize = $stackSize;
      print "$3: $stackSize\n";
      $dump = 1;
    }
    $func{$1} = "$3: $stackSize";
  }
  elsif(/(0x[0-9a-f]{8})/)
  {
    if ($dump)
    {
      print " called from $func{$1}\n";
    }
  }
}
__DATA__
