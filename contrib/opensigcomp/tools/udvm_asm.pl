#!/usr/bin/perl -w

# ***********************************************************************
#   Open SigComp -- Implementation of RFC 3320 Signaling Compression
#
#   Copyright 2005 Estacado Systems, LLC
#
#   Your use of this code is governed by the license under which it
#   has been provided to you. Unless you have a written and signed
#   document from Estacado Systems, LLC stating otherwise, your license
#   is as provided by the GNU General Public License version 2, a copy
#   of which is available in this project in the file named "LICENSE."
#   Alternately, a copy of the licence is available by writing to
#   the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
#   Boston, MA 02110-1307 USA
#
#   Unless your use of this code is goverened by a written and signed
#   contract containing provisions to the contrary, this program is
#   distributed WITHOUT ANY WARRANTY; without even the implied warranty
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#   license for additional details.
#
#   To discuss alternate licensing terms, contact info@estacado.net
# ***********************************************************************


  #################################################################
  #
  # Current limitations (marked in code with "XXX"):
  #
  #  - Does not support omission of literal parameters
  #
  #  - Has no recovery technique for handling situation in which
  #    bytecodes fail to converge after several passes
  #
  #  - Handling of "!" doesn't even try to check whether
  #    it has found a zero in bytecodes or data.
  #
  #  - Does not handle integers greater than 0xFFFF; so, for example,
  #    if one does:
  #
  #      set (length_table_start, (((length_table - 4) + 65536) / 4))
  #
  #    (as in the example deflate assembly), then you hit an
  #    overflow. To prevent this kind of problem, this operation
  #    should instead be written as:
  #
  #      set (length_table_start, (((length_table - 4) / 4 ) + 16384))
  #
  #################################################################

$inputfile = shift || &usage;
$outputfile = shift || &usage;
$symbolfile = shift || undef($symbolfile);

open (INPUT, $inputfile) || die "$0: Could not read $inputfile: $!";
open (OUTPUT, '>'.$outputfile) || die "$0: Could not write $outputfile: $!";
binmode(OUTPUT);

&init;

while (<INPUT>)
{
  chomp;
  s/;.*//;
  LEXICAL_PARSE:
  {
    /\G[\t\r\n ]/gcx && do {redo;};
    /\G([a-z_][a-z_0-9]*)/gcx && do {&add_token($1, &NAME); redo;};
    /\G([A-Z][\-A-Z0-9]*)/gcx && do {&add_token($1, &OPCODE); redo;};

    # Delimiters
    /\G(\.)/gcx && do {&add_token($1, &DOT); redo;};
    /\G(\,)/gcx && do {&add_token($1, &COMMA); redo;};
    /\G(\!)/gcx && do {&add_token($1, &BANG); redo;};
    /\G(\$)/gcx && do {&add_token($1, &DOLLAR); redo;};
    /\G(\:)/gcx && do {&add_token($1, &COLON); redo;};
    /\G(\()/gcx && do {&add_token($1, &OPENPAREN); redo;};
    /\G(\))/gcx && do {&add_token($1, &CLOSEPAREN); redo;};

    /\G([\+\-\*\/\%\&\|\^\~]|\<\<|\>\>)/gcx && do
      {&add_token($1, &OPERATOR); redo;};

    # Numbers
    /\G(0b[0-1]+)/gcx && do {&add_token($1, &NUMBER); redo;};
    /\G(0x[0-9a-fA-F]+)/gcx && do {&add_token($1, &NUMBER); redo;};
    /\G([0-9]+)/gcx && do {&add_token($1, &NUMBER); redo;};

    /\G(.+)/gcx && do {&lexical_error($inputfile,$.,$_,$1)};
  }
}

@last_core = ();
%last_label = ();
$changed = 1;
$iterations = 0;

while ($changed && $iterations++ < 10)
{
  @undefined = ();
  foreach $token (@tokens)
  {
    &process_token($token);
  }

  # Assert that the current state is NULL.
  if ($state != &NULL)
  {
    die "inputfile: File appears to be truncated\n";
  }

  # See if the bytes have stablized
  $changed = 0;
  if ($#core != $#last_core)
  {
    $changed = 1;
  }
  else
  {
    my ($i);
    for ($i = 0; $i < $#core; $i ++)
    {
      if ($core[$i] != $last_core[$i])
      {
        $changed++;
      }
    }
  }

  # Prepare for the next pass
  @last_core = @core;
  %last_label = %label;
  &init;
}

if ($changed)
{
  # XXX
  die "$inputfile: Could not create a stable set of bytecodes. Sorry.\n";
}

# Check for undefined symbols
foreach $token (@undefined)
{
  print "$inputfile:$token->{'line'}: Undefined symbol $token->{'value'}\n";
}
if (@undefined) { die "\n"; }

@core = @last_core;

# Write the bytecodes out
for ($i = 0; $i < $#core; $i++)
{
  if ($core[$i])
  {
    $first_byte = $i;
    last;
  }
}

while ($first_byte % 64) { $first_byte--; }

if ($first_byte < 128)
{
  die "$inputfile: Bytecodes and data must start after byte 128\n";
}

$last_byte = $#core;
while (!$core[$last_byte]) { $last_byte--; }

$code_length = $last_byte - $first_byte + 1;

print "Writing $code_length bytes (plus 2 byte header) to $outputfile\n";

print OUTPUT pack("CC", ($code_length >> 4), 
                  (($code_length << 4) | (($first_byte / 64) - 1)) & 0xFF);

for ($i = $first_byte; $i <= $last_byte; $i++)
{
  print OUTPUT pack("C", $core[$i]);
}


close (OUTPUT);
close (INPUT);

if (defined($symbolfile))
{
  open (SYMBOLS, ">".$symbolfile) || die "Could not write to $symbolfile";
  foreach $symbol (sort {$last_label{$a} <=> $last_label{$b}} keys %last_label)
  {
    print SYMBOLS "$symbol = $last_label{$symbol}\n";
  }
  close (SYMBOLS);
}

###########################################################################

sub add_token
{
  my ($token, $type) = @_;
  push (@tokens, {'value' => $token, 'type' => $type, 'line' => $. });
}

sub process_token
{
  my ($token) = shift;
  my ($value, $type) = ($token->{'value'},$token->{'type'});
  my ($oldstate) = $state;

  if (defined ($state [$state][$type]))
  {
    push(@pending_tokens, $token);

    $state = $state[$state][$type];
    if (defined ($action[$oldstate][$type]))
    {
      eval($action[$oldstate][$type]) || die "Aborting assembly $!\n";
    }
  }
  else
  {
    die "$inputfile:".$token->{'line'}.
        ": Syntax error; token '$value' unexpected here\n";
  }
}

sub emit_opcode
{
  my ($token) = shift @pending_tokens;
  my ($opcode) = $token->{'value'};
  my ($parameter);
  my ($paramnum) = 0;

  $curr = $pc;

  if (!exists $opcode{$opcode})
  {
    print "$inputfile:".$token->{'line'}.": Unknown opcode '$opcode'";
    die "\n";
  }

  $core[$curr] = $opcode{$opcode}{'bytecode'};
  $curr++;

  if ($pending_tokens[0]{'type'} == &OPENPAREN)
  {
    shift @pending_tokens;
  }
  else
  {
    $pc = $curr;
    return 1;
  }

  foreach $param_type (@{$opcode{$opcode}{'parameters'}})
  {
    if ($token->{'type'} == &CLOSEPAREN)
    {
      print "$inputfile:".$token->{'line'}.
            ": Too few parameters to opcode $opcode\n";
      die "\n";
    }
    $paramnum++;
    $token = &encode_parameter($param_type, $paramnum);
  }

  while ($repeat && @{$opcode{$opcode}{'repeat'}})
  {
    foreach $param_type (@{$opcode{$opcode}{'repeat'}})
    {
      if ($token->{'type'} == &CLOSEPAREN)
      {
        print "$inputfile:".$token->{'line'}.
              ": Invalid number of repeating parameters to opcode $opcode\n";
        die "\n";
      }
      $paramnum++;
      $token = &encode_parameter($param_type, $paramnum);
    }
    $repeat--;
  }

  if ($token->{'type'} != &CLOSEPAREN)
  {
    print "$inputfile:".$token->{'line'}.
          ": Too many parameters to opcode $opcode\n";
    die "\n";
  }

  @pending_tokens = ();
  $pc = $curr;
  1;
}

sub encode_parameter
{
  my ($param_type, $paramnum) = shift;
  my ($token);

  $token = $pending_tokens[0];
  if ($param_type == &LITERAL)
  {
    if ($token->{'type'} == &COMMA)
    {
      # XXX
      print "$inputfile:".$token->{'line'}.
            ": This version of the assembler is too lazy to count your ".
            "parameters for you. Sorry.\n";
      die "\n";
    }

    if ($token->{'type'} == &DOLLAR)
    {
      print "$inputfile:".$token->{'line'}.
            ": Parameter #$paramnum is literal; ".
            "the '\$' modifier is illegal\n";
      die "\n";
    }
    my ($value) = &evaluate_expression;
    &encode_literal($value);
    $repeat = $value - 1;
  }
  if ($param_type == &REFERENCE)
  {
    if ($token->{'type'} != &DOLLAR)
    {
      print "$inputfile:".$token->{'line'}.
            ": Parameter #$paramnum is of type reference; ".
            "the '\$' modifier is mandatory\n";
      die "\n";
    }
    shift (@pending_tokens);
    &encode_reference(&evaluate_expression);
  }
  if ($param_type == &MULTITYPE)
  {
    if ($token->{'type'} == &DOLLAR)
    {
      shift (@pending_tokens);
      &encode_multitype_pointer(&evaluate_expression);
    }
    else
    {
      &encode_multitype(&evaluate_expression);
    }
  }
  if ($param_type == &ADDRESS)
  {
    if ($token->{'type'} == &DOLLAR)
    {
      shift (@pending_tokens);
      &encode_address_pointer(&evaluate_expression);
    }
    else
    {
      &encode_address(&evaluate_expression);
    }
  }

  $token = shift @pending_tokens; # comma or closeparen
  return $token;
}

sub encode_literal
{
  my ($parameter) = shift;

  if (!defined($parameter))
  {
    $core[$curr] = 0xFF;
    $core[$curr+1] = 0xFF;
    $core[$curr+2] = 0xFF;
    $curr += 3;
  }
  elsif ($parameter < 128)
  {
    $core[$curr] = $parameter;
    $curr++;
  }
  elsif ($parameter < 16384)
  {
    $core[$curr] = ($parameter >> 8) | 0x80;
    $core[$curr+1] = $parameter & 0xFF;
    $curr += 2;
  }
  else
  {
    $core[$curr] = 0xC0;
    $core[$curr+1] = $parameter >> 8;
    $core[$curr+2] = $parameter & 0xFF;
    $curr += 3;
  }
}

sub encode_reference
{
  my ($parameter) = shift;

  if (!defined($parameter))
  {
    $core[$curr] = 0xFF;
    $core[$curr+1] = 0xFF;
    $core[$curr+2] = 0xFF;
    $curr += 3;
  }
  elsif ($parameter < 256 && !($parameter & 0x01))
  {
    $parameter >>= 1;
    $core[$curr] = $parameter;
    $curr++;
  }
  elsif ($parameter < 32768 && !($parameter & 0x01))
  {
    $parameter >>= 1;
    $core[$curr] = ($parameter >> 8) | 0x80;
    $core[$curr+1] = $parameter & 0xFF;
    $curr += 2;
  }
  else
  {
    $core[$curr] = 0xC0;
    $core[$curr+1] = $parameter >> 8;
    $core[$curr+2] = $parameter & 0xFF;
    $curr += 3;
  }
}

sub encode_multitype
{
  my ($parameter) = shift;

  if (!defined($parameter))
  {
    $core[$curr] = 0xFF;
    $core[$curr+1] = 0xFF;
    $core[$curr+2] = 0xFF;
    $curr += 3;
  }
  elsif($parameter < 64)
  {
    $core[$curr] = $parameter;
    $curr++;
  }
  elsif($parameter == 64)
  {
    $core[$curr] = 0x86;
    $curr++;
  }
  elsif($parameter == 128)
  {
    $core[$curr] = 0x87;
    $curr++;
  }
  elsif($parameter == 256)
  {
    $core[$curr] = 0x88;
    $curr++;
  }
  elsif($parameter == 512)
  {
    $core[$curr] = 0x89;
    $curr++;
  }
  elsif($parameter == 1024)
  {
    $core[$curr] = 0x8a;
    $curr++;
  }
  elsif($parameter == 2048)
  {
    $core[$curr] = 0x8b;
    $curr++;
  }
  elsif($parameter == 4096)
  {
    $core[$curr] = 0x8c;
    $curr++;
  }
  elsif($parameter == 8192)
  {
    $core[$curr] = 0x8d;
    $curr++;
  }
  elsif($parameter == 16384)
  {
    $core[$curr] = 0x8e;
    $curr++;
  }
  elsif($parameter == 32768)
  {
    $core[$curr] = 0x8f;
    $curr++;
  }
  elsif($parameter >= 65504)
  {
    $core[$curr] = ($parameter - 65504) | 0xe0;
    $curr++;
  }
  elsif($parameter > 61440)
  {
    $parameter -= 61440;
    $core[$curr] = ($parameter >> 8) | 0x90;
    $core[$curr + 1] = $parameter & 0xFF;
    $curr += 2;
  }
  elsif($parameter < 8192)
  {
    $core[$curr] = ($parameter >> 8) | 0xa0;
    $core[$curr + 1] = $parameter & 0xFF;
    $curr += 2;
  }
  else
  {
    $core[$curr] = 0x80;
    $core[$curr + 1] = ($parameter >> 8);
    $core[$curr + 2] = $parameter & 0xFF;
    $curr += 3;
  }
}

sub encode_multitype_pointer
{
  my ($parameter) = shift;

  if (!defined($parameter))
  {
    $core[$curr] = 0xFF;
    $core[$curr+1] = 0xFF;
    $core[$curr+2] = 0xFF;
    $curr += 3;
  }
  elsif ($parameter < 128 && !($parameter & 0x01))
  {
    $parameter >>= 1;
    $core[$curr] = $parameter | 0x40;
    $curr++;
  }
  elsif ($parameter < 8192)
  {
    $core[$curr] = ($parameter >> 8) | 0xc0;
    $core[$curr+1] = $parameter & 0xff;
    $curr += 2;
  }
  else
  {
    $core[$curr] = 0x81;
    $core[$curr+1] = $parameter >> 8;
    $core[$curr+2] = $parameter & 0xFF;
    $curr += 3;
  }
}

sub encode_address
{
  my ($parameter) = shift;

  if (!defined($parameter))
  {
    $core[$curr] = 0xFF;
    $core[$curr+1] = 0xFF;
    $core[$curr+2] = 0xFF;
    $curr += 3;
  }
  else
  {
    &encode_multitype(($parameter - $pc) % 0x10000);
  }
}

sub encode_address_pointer;
{
  my ($parameter) = shift;

  if (!defined($parameter))
  {
    $core[$curr] = 0xFF;
    $core[$curr+1] = 0xFF;
    $core[$curr+2] = 0xFF;
    $curr += 3;
  }
  else
  {
    &encode_multitype_pointer(($parameter - $pc) % 0x10000);
  }
}

sub enact_directive
{
  my ($token);
  my ($directive);
  my ($i);

  $token = shift @pending_tokens;
  $directive = $token->{'value'};

  # kill the opening parenthesis
  shift @pending_tokens;

  if ($directive eq 'pad')
  {
    # Insert (parameter) bytes
    my ($padding) = &evaluate_expression;
    for ($i = $pc; $i < ($pc + $padding); $i++)
    {
      $core[$i] = 0;
    }
    $pc += $padding;
  }
  elsif ($directive eq 'align')
  {
    # Insert bytes sufficient to acheive specified alignment
    my ($alignment) = &evaluate_expression;
    my ($padding) = $alignment - ($pc % $alignment);

    for ($i = $pc; $i < ($pc + $padding); $i++)
    {
      $core[$i] = 0;
    }
    $pc += $padding;
  }
  elsif ($directive eq 'at')
  {
    # Insert bytes sufficient to reach parameter. If we are
    # past that address, emit an error
    my ($position) = &evaluate_expression;

    if ($position < $pc)
    {
      print "$inputfile:".$token->{'line'}.
            ": Parameter is too small; must be $pc or larger.";
      die "\n";
    }

    for ($i = $pc; $i < $position; $i++)
    {
      $core[$i] = 0;
    }
    $pc = $position;
  }
  elsif ($directive eq 'byte')
  {
    my ($curr) = $pc;
    # Append all specified parameters as bytes
    while ($pending_tokens[0]->{'type'} != &CLOSEPAREN)
    {
      if ($pending_tokens[0]->{'type'} == &COMMA)
      {
        shift @pending_tokens;
        next;
      }
      $core[$curr] = &evaluate_expression & 0xFF;
      $curr++;
    }
    $pc = $curr;
  }
  elsif ($directive eq 'word')
  {
    # Append all specified parameters as words
    my ($curr) = $pc;
    my ($val);
    while ($pending_tokens[0]->{'type'} != &CLOSEPAREN)
    {
      if ($pending_tokens[0]->{'type'} == &COMMA)
      {
        shift @pending_tokens;
        next;
      }
      $val = &evaluate_expression;
      $core[$curr] = ($val >> 8) & 0xFF;
      $core[$curr+1] = $val & 0xFF;
      $curr+= 2;
    }
    $pc = $curr;
  }
  elsif ($directive eq 'set')
  {
    # Set the label specified in the first parameter to the
    # value specified in the second parameter
    # Get label name
    $token = shift (@pending_tokens);
    my ($name) = $token->{'value'};

    if (exists $label{$name})
    {
      print "$inputfile:".$token->{'line'}.": Duplicate label '$name' defined".
            " (previous definition on line $linenum{$name})";
      die "\n";
    }
    else
    {
      $token = shift (@pending_tokens);
      if ($token->{'type'} != &COMMA)
      {
        print "$inputfile:".$token->{'line'}.
              ": Too few parameters to '$directive' directive\n";
        die "\n";
      }
      $linenum{$name} = $token->{'line'};
      $label{$name} = &evaluate_expression;
    }
  }
  else
  {
    print "$inputfile:".$token->{'line'}.": Unknown directive '$directive'";
    die "\n";
  }

  # There should only be a closing paren left
  if (@pending_tokens > 1)
  {
    print "$inputfile:".$token->{'line'}.
          ": Too many parameters to '$directive' directive\n";
    die "\n";
  }
  @pending_tokens = ();
  1;
}

sub evaluate_expression
{
  my ($token);
  $token = shift @pending_tokens;
  if ($token->{'type'} eq &OPENPAREN)
  {
    my ($left, $right, $expression);
    $left = &evaluate_expression;
    $token = shift @pending_tokens; # operator
    $right = &evaluate_expression;
    shift @pending_tokens;          # close paren
    if (!defined($left) || !defined($right))
    {
      return undef;
    }
    $expression = "$left $token->{'value'} $right";
    return (eval $expression) % 0x10000;
  }
  elsif ($token->{'type'} eq &NAME)
  {
    if (defined $label{$token->{'value'}})
    {
      return $label{$token->{'value'}} % 65536;
    }
    elsif (exists $last_label{$token->{'value'}})
    {
      return $last_label{$token->{'value'}} % 65536;
    }
    else
    {
      push (@undefined, $token);
      return undef;
    }
  }
  elsif ($token->{'type'} eq &NUMBER)
  {
    my ($number) = $token->{'value'};
    if ($number =~ /^0x/)
    {
      return eval($number) % 65536;
    }
    elsif ($number =~ /^0b/)
    {
      return &binary($number) % 65536;
    }
    else
    {
      return $number % 65536;
    }
  }
  elsif ($token->{'type'} eq &DOT)
  {
    return $pc;
  }

  # XXX We really should make sure that we haven't
  # backed into data here.

  elsif ($token->{'type'} eq &BANG)
  {
    my ($fail) = $pc-1;
    while ($fail > 0)
    {
      if ($core[$fail] == 0)
      {
        return $fail;
      }
      $fail--;
    }
    return undef;
  }
  else
  {
    print "$inputfile:".$token->{'line'}.": Syntax error in expression;".
          " token '".$token{'value'}." unexpected here\n";
    die;
  }
}

sub record_label
{
  my ($token, $name);

  # Eat colon
  shift (@pending_tokens);

  # Get label name
  $token = shift (@pending_tokens);
  $name = $token->{'value'};

  if (exists $label{$name})
  {
    print "$inputfile:".$token->{'line'}.": Duplicate label '$name' defined".
          " (previous definition on line $linenum{$name})";
    die "\n";
  }
  else
  {
    $label{$name} = $pc;
    $linenum{$name} = $token->{'line'};
  }
  1;
}

sub expression_complete
{
  if ($phase[$level] eq 'opcode')
  {
    $state = &OPCODE3;
  }
  elsif ($phase[$level] eq 'directive')
  {
    $state = &DIRECTIVE3;
  }
  elsif ($phase[$level] eq 'left')
  {
    $state = &EXPRESSION2;
  }
  elsif ($phase[$level] eq 'right')
  {
    $state = &EXPRESSION4;
  }
  else
  {
    die "Internal error";
  }
  $level--;
}

sub binary
{
  my ($bits) = shift;

  if (length($bits) > 18)
  {
    die "$inputfile:$.: Error: '$bits' is more than 16 bits long\n";
  }

  $bits =~ s/^0b//;

  $bits = ('0' x (16 - length($bits))) . $bits;
  return (unpack("n",pack("B16",$bits)));
}

sub lexical_error
{
  my ($file, $linenum, $line, $remainder) = @_;
  my ($column);
  $column = length($line) - length ($remainder);
  print "$file:$linenum: Error during lexical parsing ".
        "(this character is not allowed here)\n";
  print "  $line\n";
  print "  ".(' ' x $column)."^\n";
  die "\n";
}

sub usage
{
  die "usage: $0 <infile> <outfile> [symbolfile]\n";
}

# States
sub NULL        {0}
sub OPCODE1     {1}
sub OPCODE2     {2}
sub OPCODE3     {3}
sub OPCODE4     {4}
sub DIRECTIVE1  {5}
sub DIRECTIVE2  {6}
sub DIRECTIVE3  {7}
sub LABEL       {8}
sub EXPRESSION1 {9}
sub EXPRESSION2 {10}
sub EXPRESSION3 {11}
sub EXPRESSION4 {12}

# Token types
sub NAME        {0}
sub OPCODE      {1}
sub DOT         {2}
sub COMMA       {3}
sub BANG        {4}
sub DOLLAR      {5}
sub COLON       {6}
sub OPENPAREN   {7}
sub CLOSEPAREN  {8}
sub OPERATOR    {9}
sub NUMBER      {10}

# Parameter Types
sub LITERAL     {0}
sub REFERENCE   {1}
sub MULTITYPE   {2}
sub ADDRESS     {3}

sub init
{
  @pending_tokens = ();
  %label = ();
  $pc = 0;
  @core = ();

  # State Transitions
  $state [&NULL][&OPCODE] = &OPCODE1;
  $state [&NULL][&NAME] = &DIRECTIVE1;
  $state [&NULL][&COLON] = &LABEL;

  $state [&OPCODE1][&OPCODE] = &OPCODE1;
  $action[&OPCODE1][&OPCODE] = "&emit_opcode";
  $state [&OPCODE1][&NAME] = &DIRECTIVE1;
  $action[&OPCODE1][&NAME] = "&emit_opcode";
  $state [&OPCODE1][&COLON] = &LABEL;
  $action[&OPCODE1][&COLON] = "&emit_opcode";
  $state [&OPCODE1][&OPENPAREN] = &OPCODE2;

  $state [&OPCODE2][&COMMA] = &OPCODE2;
  $state [&OPCODE2][&NAME] = &OPCODE3;
  $state [&OPCODE2][&NUMBER] = &OPCODE3;
  $state [&OPCODE2][&DOT] = &OPCODE3;
  $state [&OPCODE2][&BANG] = &OPCODE3;
  $state [&OPCODE2][&DOLLAR] = &OPCODE4;
  $state [&OPCODE2][&OPENPAREN] = &EXPRESSION1;
  $action[&OPCODE2][&OPENPAREN] = "\$level++;\$phase[\$level]='opcode'";

  $state [&OPCODE3][&COMMA] = &OPCODE2;
  $state [&OPCODE3][&CLOSEPAREN] = &NULL;
  $action[&OPCODE3][&CLOSEPAREN] = "&emit_opcode";

  $state [&OPCODE4][&NAME] = &OPCODE3;
  $state [&OPCODE4][&NUMBER] = &OPCODE3;
  $state [&OPCODE4][&DOT] = &OPCODE3;
  $state [&OPCODE4][&BANG] = &OPCODE3;
  $state [&OPCODE4][&OPENPAREN] = &EXPRESSION1;
  $action[&OPCODE4][&OPENPAREN] = "\$level++;\$phase[\$level]='opcode'";

  $state [&DIRECTIVE1][&OPENPAREN] = &DIRECTIVE2;

  $state [&DIRECTIVE2][&NAME] = &DIRECTIVE3;
  $state [&DIRECTIVE2][&NUMBER] = &DIRECTIVE3;
  $state [&DIRECTIVE2][&DOT] = &DIRECTIVE3;
  $state [&DIRECTIVE2][&BANG] = &DIRECTIVE3;
  $state [&DIRECTIVE2][&OPENPAREN] = &EXPRESSION1;
  $action[&DIRECTIVE2][&OPENPAREN] = "\$level++;\$phase[\$level]='directive'";

  $state [&DIRECTIVE3][&COMMA] = &DIRECTIVE2;
  $state [&DIRECTIVE3][&CLOSEPAREN] = &NULL;
  $action[&DIRECTIVE3][&CLOSEPAREN] = "&enact_directive";

  $state [&LABEL][&NAME] = &NULL;
  $action[&LABEL][&NAME] = "&record_label";

  $state [&EXPRESSION1][&OPENPAREN] = &EXPRESSION1;
  $action[&EXPRESSION1][&OPENPAREN] = "\$level++;\$phase[\$level]='left'";
  $state [&EXPRESSION1][&NAME] = &EXPRESSION2;
  $state [&EXPRESSION1][&NUMBER] = &EXPRESSION2;
  $state [&EXPRESSION1][&DOT] = &EXPRESSION2;
  $state [&EXPRESSION1][&BANG] = &EXPRESSION2;

  $state [&EXPRESSION2][&OPERATOR] = &EXPRESSION3;

  $state [&EXPRESSION3][&OPENPAREN] = &EXPRESSION1;
  $action[&EXPRESSION3][&OPENPAREN] = "\$level++;\$phase[\$level]='right'";
  $state [&EXPRESSION3][&NAME] = &EXPRESSION4;
  $state [&EXPRESSION3][&NUMBER] = &EXPRESSION4;
  $state [&EXPRESSION3][&DOT] = &EXPRESSION4;
  $state [&EXPRESSION3][&BANG] = &EXPRESSION4;
  
  $state [&EXPRESSION4][&CLOSEPAREN] = &EXPRESSION4; # Not really
  $action[&EXPRESSION4][&CLOSEPAREN] = "&expression_complete";
  
  $state = &NULL;
  $level = 0;

  %opcode =
  (
    'DECOMPRESSION-FAILURE' =>
    {
      'bytecode' => 0,
      'parameters' => [],
    },
    'AND' =>
    {
      'bytecode' => 1,
      'parameters' => [&REFERENCE, &MULTITYPE],
    },
    'OR' =>
    {
      'bytecode' => 2,
      'parameters' => [&REFERENCE, &MULTITYPE],
    },
    'NOT' =>
    {
      'bytecode' => 3,
      'parameters' => [&REFERENCE],
    },
    'LSHIFT' =>
    {
      'bytecode' => 4,
      'parameters' => [&REFERENCE, &MULTITYPE],
    },
    'RSHIFT' =>
    {
      'bytecode' => 5,
      'parameters' => [&REFERENCE, &MULTITYPE],
    },
    'ADD' =>
    {
      'bytecode' => 6,
      'parameters' => [&REFERENCE, &MULTITYPE],
    },
    'SUBTRACT' =>
    {
      'bytecode' => 7,
      'parameters' => [&REFERENCE, &MULTITYPE],
    },
    'MULTIPLY' =>
    {
      'bytecode' => 8,
      'parameters' => [&REFERENCE, &MULTITYPE],
    },
    'DIVIDE' =>
    {
      'bytecode' => 9,
      'parameters' => [&REFERENCE, &MULTITYPE],
    },
    'REMAINDER' =>
    {
      'bytecode' => 10,
      'parameters' => [&REFERENCE, &MULTITYPE],
    },
    'SORT-ASCENDING' =>
    {
      'bytecode' => 11,
      'parameters' => [&MULTITYPE, &MULTITYPE, &MULTITYPE],
    },
    'SORT-DESCENDING' =>
    {
      'bytecode' => 12,
      'parameters' => [&MULTITYPE, &MULTITYPE, &MULTITYPE],
    },
    'SHA-1' =>
    {
      'bytecode' => 13,
      'parameters' => [&MULTITYPE, &MULTITYPE, &MULTITYPE],
    },
    'LOAD' =>
    {
      'bytecode' => 14,
      'parameters' => [&MULTITYPE, &MULTITYPE],
    },
    'MULTILOAD' =>
    {
      'bytecode' => 15,
      'parameters' => [&MULTITYPE, &LITERAL, &MULTITYPE],
      'repeat' => [&MULTITYPE]
    },
    'PUSH' =>
    {
      'bytecode' => 16,
      'parameters' => [&MULTITYPE],
    },
    'POP' =>
    {
      'bytecode' => 17,
      'parameters' => [&MULTITYPE],
    },
    'COPY' =>
    {
      'bytecode' => 18,
      'parameters' => [&MULTITYPE, &MULTITYPE, &MULTITYPE],
    },
    'COPY-LITERAL' =>
    {
      'bytecode' => 19,
      'parameters' => [&MULTITYPE, &MULTITYPE, &REFERENCE],
    },
    'COPY-OFFSET' =>
    {
      'bytecode' => 20,
      'parameters' => [&MULTITYPE, &MULTITYPE, &REFERENCE],
    },
    'MEMSET' =>
    {
      'bytecode' => 21,
      'parameters' => [&MULTITYPE, &MULTITYPE, &MULTITYPE, &MULTITYPE],
    },
    'JUMP' =>
    {
      'bytecode' => 22,
      'parameters' => [&ADDRESS],
    },
    'COMPARE' =>
    {
      'bytecode' => 23,
      'parameters' => [&MULTITYPE, &MULTITYPE, &ADDRESS, &ADDRESS, &ADDRESS],
    },
    'CALL' =>
    {
      'bytecode' => 24,
      'parameters' => [&ADDRESS],
    },
    'RETURN' =>
    {
      'bytecode' => 25,
      'parameters' => [],
    },
    'SWITCH' =>
    {
      'bytecode' => 26,
      'parameters' => [&LITERAL, &MULTITYPE, &ADDRESS, &ADDRESS],
      'repeat' => [&MULTITYPE]
    },
    'CRC' =>
    {
      'bytecode' => 27,
      'parameters' => [&MULTITYPE, &MULTITYPE, &MULTITYPE, &ADDRESS],
    },
    'INPUT-BYTES' =>
    {
      'bytecode' => 28,
      'parameters' => [&MULTITYPE, &MULTITYPE, &ADDRESS],
    },
    'INPUT-BITS' =>
    {
      'bytecode' => 29,
      'parameters' => [&MULTITYPE, &MULTITYPE, &ADDRESS],
    },
    'INPUT-HUFFMAN' =>
    {
      'bytecode' => 30,
      'parameters' => [&MULTITYPE, &ADDRESS, &LITERAL, &MULTITYPE, &MULTITYPE,
                       &MULTITYPE, &MULTITYPE],
      'repeat' => [&MULTITYPE, &MULTITYPE, &MULTITYPE, &MULTITYPE]
    },
    'STATE-ACCESS' =>
    {
      'bytecode' => 31,
      'parameters' => [&MULTITYPE, &MULTITYPE, &MULTITYPE, &MULTITYPE,
                       &MULTITYPE, &MULTITYPE],
    },
    'STATE-CREATE' =>
    {
      'bytecode' => 32,
      'parameters' => [&MULTITYPE, &MULTITYPE, &MULTITYPE, &MULTITYPE,
                       &MULTITYPE],
    },
    'STATE-FREE' =>
    {
      'bytecode' => 33,
      'parameters' => [&MULTITYPE, &MULTITYPE],
    },
    'OUTPUT' =>
    {
      'bytecode' => 34,
      'parameters' => [&MULTITYPE, &MULTITYPE],
    },
    'END-MESSAGE' =>
    {
      'bytecode' => 35,
      'parameters' => [&MULTITYPE, &MULTITYPE, &MULTITYPE, &MULTITYPE,
                       &MULTITYPE, &MULTITYPE, &MULTITYPE],
    },
  );
}
