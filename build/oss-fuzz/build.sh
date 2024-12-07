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

cmake -S . -B _build \
    -GNinja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_CLICKTOCALL=OFF \
    -DBUILD_QPID_PROTON=OFF \
    -DBUILD_RECON=OFF \
    -DBUILD_REFLOW=OFF \
    -DBUILD_REND=OFF \
    -DBUILD_REPRO=OFF \
    -DBUILD_RETURN=OFF \
    -DBUILD_TFM=OFF \
    -DREGENERATE_MEDIA_SAMPLES=OFF \
    -DUSE_DTLS=OFF \
    -DUSE_KURENTO=OFF \
    -DUSE_MAXMIND_GEOIP=OFF \
    -DUSE_POPT=OFF \
    -DWITH_SSL=OFF

cmake --build _build --target fuzzUtil fuzzStack aresfuzz aresfuzzname

cp _build/rutil/test/fuzzUtil $OUT/
cp _build/resip/stack/test/fuzzStack $OUT/

# those fuzz targets are too small
#  See: https://github.com/google/oss-fuzz/issues/1331
#cp _build/rutil/dns/ares/{aresfuzz,aresfuzzname} $OUT/
