/*
   Licensed to the Apache Software Foundation (ASF) under one or more
   contributor license agreements.  See the NOTICE file distributed with
   this work for additional information regarding copyright ownership.
   The ASF licenses this file to You under the Apache License, Version 2.0
   (the "License"); you may not use this file except in compliance with
   the License.  You may obtain a copy of the License at

       https://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
Native parser regression tests
==============================

From src/native/unix, configure and run the tests:

    sh support/buildconf.sh
    ./configure --with-java=$JAVA_HOME
    make check

The tests compile a separate executable from test_home.c and the production
parser sources. The normal make target does not build tests. No test code is
linked into jsvc or libservice.a. make clean removes the test executable;
make distclean also removes the generated test Makefile.

On compilers supporting AddressSanitizer and UndefinedBehaviorSanitizer:

    make -C tests clean
    make check EXTRA_CFLAGS="-g -O1 -fsanitize=address,undefined" EXTRA_LDFLAGS="-fsanitize=address,undefined"

The shell runner creates temporary Java homes under /tmp and removes them on
exit. Empty library files suffice because home() checks their existence without
loading them. Fixtures use the platform's production library-location patterns.
Tests cover empty input, ordinary input, the 255-entry boundary, rejection of
256 and 257 entries, duplicate definitions, skipped missing libraries, entry
order, library paths, and the terminating NULL pointer.
