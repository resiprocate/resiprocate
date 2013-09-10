#!/bin/sh
#
# Insert a test packet into a unit test.
#   ./insert-packet.sh <plaintext packet file> <C++ source>
#
# searches for the initNetwork() line and inserts just below that
#

function die() {
	echo "$1" >&2;
	exit 1;
}

test ! -z "$1" -a ! -z "$2" || die "usage: $0 input output";

quoted=`sed -e 's/\"/\\\"/g' \
            -e 's/^/         \"/' \
	    -e $'s/\r$//g' -e 's/$/\\\\r\\\\n\"/g' <$1`;

# Figure out where we want to insert this.
line=`egrep -n 'initNetwork\(\)\;$' <$2 | sed -e 's/:/ /' | awk '{print $1}'`;

total=`wc -l $2 | awk '{print $1}'`;

head -n ${line} $2 >.tmp;

cat >>.tmp <<EOF

   {
      Data txt(
${quoted}
      );

      std::auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));

      try
      {
         (void)message->header(h_From);
      }
      catch (ParseException&)
      {
         std::cerr << "uh, failed to parse message: "
	           << __FILE__ << ':' << __LINE__
		   << std::endl;
         assert(false);
      }
   }
EOF

tail -n `expr ${total} - ${line}` $2 >>.tmp;

mv .tmp $2;

echo "inserted at line `expr ${line} + 1`";
