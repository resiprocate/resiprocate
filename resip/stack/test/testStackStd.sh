#!/bin/sh

# This is helper script used from runtests.sh as part of the standard
# regression tests.

set -e

echo "Running TCP REGISTER test"
./testStack --protocol=tcp
echo "Running UDP REGISTER test"
./testStack --protocol=udp
echo "Running TCP REGISTER test with 50 ports"
./testStack --protocol=tcp --numports=50
echo "Running TCP INVITE test"
./testStack --protocol=tcp --invite
