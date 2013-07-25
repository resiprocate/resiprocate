#!/bin/bash

# This script changes various types such as UInt32 and u_int32_t into
# the C++11 equivalents such as uint32_t

find resip repro tfm -type f -name '*.[ch]xx' -exec sed -i 's/::u_int32_t/u_int32_t/g' {} \;

find resip repro tfm -type f -name '*.[ch]xx' -exec sed -i 's/u_int32_t/uint32_t/g' {} \;

find rutil -type f -name '*.[ch]xx' -exec sed -i 's/u_int32_t/uint32_t/g' {} \;

find . -type f -name '*.[ch]xx' -exec sed -i 's/\([^a-zA-Z0-9]\)\(::UInt8\)\([^a-zA-Z0-9]\)/\1uint8_t\3/g' {} \;
find . -type f -name '*.[ch]xx' -exec sed -i 's/\([^a-zA-Z0-9]\)\(::UInt16\)\([^a-zA-Z0-9]\)/\1uint16_t\3/g' {} \;
find . -type f -name '*.[ch]xx' -exec sed -i 's/\([^a-zA-Z0-9]\)\(::UInt32\)\([^a-zA-Z0-9]\)/\1uint32_t\3/g' {} \;
find . -type f -name '*.[ch]xx' -exec sed -i 's/\([^a-zA-Z0-9]\)\(::UInt64\)\([^a-zA-Z0-9]\)/\1uint64_t\3/g' {} \;


find . -type f -name '*.[ch]xx' -exec sed -i 's/\([^a-zA-Z0-9]\)\(UInt8\)\([^a-zA-Z0-9]\)/\1uint8_t\3/g' {} \;
find . -type f -name '*.[ch]xx' -exec sed -i 's/\([^a-zA-Z0-9]\)\(UInt16\)\([^a-zA-Z0-9]\)/\1uint16_t\3/g' {} \;
find . -type f -name '*.[ch]xx' -exec sed -i 's/\([^a-zA-Z0-9]\)\(UInt32\)\([^a-zA-Z0-9]\)/\1uint32_t\3/g' {} \;
find . -type f -name '*.[ch]xx' -exec sed -i 's/\([^a-zA-Z0-9]\)\(UInt64\)\([^a-zA-Z0-9]\)/\1uint64_t\3/g' {} \;

find . -type f -name '*.[ch]xx' -exec sed -i 's/^\(UInt8\)\([^a-zA-Z0-9]\)/uint8_t\2/g' {} \;
find . -type f -name '*.[ch]xx' -exec sed -i 's/^\(UInt16\)\([^a-zA-Z0-9]\)/uint16_t\2/g' {} \;
find . -type f -name '*.[ch]xx' -exec sed -i 's/^\(UInt32\)\([^a-zA-Z0-9]\)/uint32_t\2/g' {} \;
find . -type f -name '*.[ch]xx' -exec sed -i 's/^\(UInt64\)\([^a-zA-Z0-9]\)/uint64_t\2/g' {} \;

find . -type f -name '*.[ch]xx' -exec sed -i 's/\([^a-zA-Z0-9]\)UInt8$/\1uint8_t/g' {} \;
find . -type f -name '*.[ch]xx' -exec sed -i 's/\([^a-zA-Z0-9]\)UInt16$/\1uint16_t/g' {} \;
find . -type f -name '*.[ch]xx' -exec sed -i 's/\([^a-zA-Z0-9]\)UInt32$/\1uint32_t/g' {} \;
find . -type f -name '*.[ch]xx' -exec sed -i 's/\([^a-zA-Z0-9]\)UInt64$/\1uint64_t/g' {} \;

find . -type f -name '*.[ch]xx' -exec sed -i 's/^UInt8$/uint8_t/g' {} \;
find . -type f -name '*.[ch]xx' -exec sed -i 's/^UInt16$/uint16_t/g' {} \;
find . -type f -name '*.[ch]xx' -exec sed -i 's/^UInt32$/uint32_t/g' {} \;
find . -type f -name '*.[ch]xx' -exec sed -i 's/^UInt64$/uint64_t/g' {} \;



