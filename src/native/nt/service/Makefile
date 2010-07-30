#
#  Licensed to the Apache Software Foundation (ASF) under one or more
#  contributor license agreements.  See the NOTICE file distributed with
#  this work for additional information regarding copyright ownership.
#  The ASF licenses this file to You under the Apache License, Version 2.0
#  (the "License"); you may not use this file except in compliance with
#  the License.  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
INCLUDE=-I../lib -I ../../unix/native
../../../../dist/INSTSVC: instmain.c arguments.o debug.o
	gcc $(INCLUDE) instmain.c   -o ../../../../dist/INSTSVC arguments.o debug.o
arguments.o: ../../unix/native/arguments.c
	gcc -c $(INCLUDE) ../../unix/native/arguments.c
debug.o: ../../unix/native/debug.c
	gcc -c $(INCLUDE) ../../unix/native/debug.c
clean:
	rm -f arguments.o debug.o
