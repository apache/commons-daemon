#!/bin/sh
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

set -eu

# Keep paths short enough for the production parser's fixed path buffers.
test_dir=$(mktemp -d /tmp/jsvc-home-test.XXXXXXXX)
trap 'rm -rf "$test_dir"' 0
trap 'exit 1' 1 2 3 15

for duplicates in 0 1; do
    for count in 0 1 254 255 256 257; do
        ./test_home "$test_dir/home-$duplicates-$count" "$count" "$duplicates"
    done
done
