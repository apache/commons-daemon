#!/bin/sh
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
if [ ".$1" = . ];then
  arch="`uname -m`"
  echo "No architecture provided. Using $arch"
else
  arch=$1
fi
topdir=.
major_sed='/#define.*JSVC_MAJOR_VERSION/s/^[^0-9]*\([0-9]*\).*$/\1/p'
minor_sed='/#define.*JSVC_MINOR_VERSION/s/^[^0-9]*\([0-9]*\).*$/\1/p'
patch_sed='/#define.*JSVC_PATCH_VERSION/s/^[^0-9]*\([0-9]*\).*$/\1/p'
vmajor="`sed -n $major_sed $topdir/native/version.h`"
vminor="`sed -n $minor_sed $topdir/native/version.h`"
vpatch="`sed -n $patch_sed $topdir/native/version.h`"
osname="`uname -s | tr [A-Z] [a-z]`"
verdst="commons-daemon-$vmajor.$vminor.$vpatch-bin-$osname-$arch"
extfiles="LICENSE.txt NOTICE.txt RELEASE-NOTES.txt"
for i in $extfiles
do
  cp ../../../$i .
done
tar cfz $verdst.tar.gz jsvc $extfiles
if [ ".$?" = .0 ]; then
  md5sum --binary $verdst.tar.gz > $verdst.tar.gz.md5
  sha1sum --binary $verdst.tar.gz > $verdst.tar.gz.sha1
else
  rm $verdst.tar.gz >/dev/null 2>&1
fi
rm $extfiles
