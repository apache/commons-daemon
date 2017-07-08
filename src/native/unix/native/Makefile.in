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

include ../Makedefs

OBJS =  arguments.o \
	debug.o \
	dso-dlfcn.o \
	dso-dyld.o \
	help.o \
	home.o \
	java.o \
	location.o \
	replace.o \
	locks.o \
	signals.o

all: jsvc libservice.a

libservice.a: $(OBJS)
	ar cr libservice.a $(OBJS)
	$(RANLIB) libservice.a

jsvc: jsvc-unix.o libservice.a
	$(LDCMD) $(LDFLAGS) $(EXTRA_LDFLAGS) jsvc-unix.o libservice.a $(LIBS) -o ../jsvc
	$(STRIP) ../jsvc

clean:
	rm -f $(OBJS) ../jsvc jsvc-unix.o libservice.a

