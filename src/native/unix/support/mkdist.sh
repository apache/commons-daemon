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
# Create Apache Commons Daemon native package distribution on UNIX systems.
# You should execute this script from the src/native/unix directory
#
# support/mkdist.sh <arch> [os]
#
# This will create something like commons-daemon-1.2.3-bin-os-arch.tar.gz
# The version numbers are parsed from the native/version.h file.
# If the os argument is not provided current os name will be used.
#
#
gpgopts="-ba"
arch=""
for o
do
    case "$o" in
    *=*) a=`echo "$o" | sed 's/^[-_a-zA-Z0-9]*=//'`
     ;;
    *) a=''
     ;;
    esac
    case "$o" in
        --passphrase=*  )
            gpgopts="$gpgopts --passphrase $a"
            shift
        ;;
        --arch=*  )
            arch="$a"
            shift
        ;;
        --os=*  )
            osname="$a"
            shift
        ;;
        * )
            break
        ;;
    esac
done


if [ ".$arch" = . ];then
  arch=`uname -m 2>/dev/null | tr '[A-Z]' '[a-z]'` || arch="unknown"
  echo "No architecture provided. Using $arch"
fi
if [ ".$osname" = . ];then
  osname=`uname -s 2>/dev/null | tr '[A-Z]' '[a-z]'` || osname="unknown"
  echo "No OS name provided. Using $osname"
fi
topdir=.
major_sed='/#define.*JSVC_MAJOR_VERSION/s/^[^0-9]*\([0-9]*\).*$/\1/p'
minor_sed='/#define.*JSVC_MINOR_VERSION/s/^[^0-9]*\([0-9]*\).*$/\1/p'
patch_sed='/#define.*JSVC_PATCH_VERSION/s/^[^0-9]*\([0-9]*\).*$/\1/p'
vmajor="`sed -n $major_sed $topdir/native/version.h`"
vminor="`sed -n $minor_sed $topdir/native/version.h`"
vpatch="`sed -n $patch_sed $topdir/native/version.h`"
verdst="commons-daemon-$vmajor.$vminor.$vpatch-bin-$osname-$arch"
extfiles="LICENSE.txt NOTICE.txt RELEASE-NOTES.txt"
for i in $extfiles
do
  cp ../../../$i .
done
# Try to locate a MD5 binary
md5_bin="`which md5sum 2>/dev/null || type md5sum 2>&1`"
if [ -x "$md5_bin" ]; then
    MD5SUM="$md5_bin --binary "
else
    MD5SUM="echo 00000000000000000000000000000000 "
fi
# Try to locate a SHA1 binary
sha1_bin="`which sha1sum 2>/dev/null || type sha1sum 2>&1`"
if [ -x "$sha1_bin" ]; then
    SHA1SUM="$sha1_bin --binary "
else
    SHA1SUM="echo 0000000000000000000000000000000000000000 "
fi
dstfile=$verdst.tar.gz
echo "Creating $dstfile ..."
tar cfz $dstfile jsvc $extfiles
if [ ".$?" = .0 ]; then
  echo "Signing $dstfile"
  gpg $gpgopts $dstfile
  $MD5SUM $dstfile > $dstfile.md5
  $SHA1SUM $dstfile > $dstfile.sha1
else
  rm $verdst.tar.gz >/dev/null 2>&1
fi
rm $extfiles
