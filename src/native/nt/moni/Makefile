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
INCLUDE=-I../lib
BIN=../../../../dist
all: $(BIN)/vdmoniadm $(BIN)/vdmonisvc

$(BIN)/vdmoniadm: vdmoniadm.c
	gcc $(INCLUDE) -I../executables/vdmoniadm  vdmoniadm.c \
	-o $(BIN)/vdmoniadm -lgdi32
$(BIN)/vdmonisvc: vdmonisvc.c vdenv.o kills.o
	gcc -mno-cygwin -I/usr/include $(INCLUDE) -I../executables/vdmonisvc \
	-DCYGWIN vdmonisvc.c \
	-o $(BIN)/vdmonisvc vdenv.o kills.o
vdenv.o: ../supcalls_nt/vdenv.c
	gcc $(INCLUDE) -c ../supcalls_nt/vdenv.c
kills.o: ../signals/kills.c
	gcc $(INCLUDE) -c ../signals/kills.c
clean:
	rm -f vdenv.o kills.o 
