#!/bin/bash -eu
# Copyright 2016 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
################################################################################

autoreconf --install
./configure --disable-shared --enable-static

make -j$(nproc) -C rutil
make -j$(nproc) -C rutil/test fuzzUtil
cp rutil/test/fuzzUtil $OUT/

make -j$(nproc) -C rutil/dns/ares aresfuzz aresfuzzname
# those fuzz targets are too small
#  See: https://github.com/google/oss-fuzz/issues/1331
#cp rutil/dns/ares/{aresfuzz,aresfuzzname} $OUT/

make -j$(nproc) -C resip/stack
make -j$(nproc) -C resip/stack/test fuzzStack
cp resip/stack/test/fuzzStack $OUT/
