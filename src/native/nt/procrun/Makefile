JAVA_HOME=c:/j2sdk1.4.1_01
INCLUDE=-I. -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/win32
BIN=.
# CFLAGS for debugging
#CFLAGS=-D_DEBUG

all: $(BIN)/procrun $(BIN)/procrunw

$(BIN)/procrun: procrun.c
	gcc -mno-cygwin $(INCLUDE) $(CFLAGS) -DPROCRUN_CONSOLE procrun.c \
	-lshlwapi \
	-o $(BIN)/procrun

$(BIN)/procrunw: procrun.c
	gcc -mno-cygwin $(INCLUDE) $(CFLAGS) -DPROCRUN_WINAPP -c procgui.c
	gcc -mno-cygwin $(INCLUDE) $(CFLAGS) -DPROCRUN_WINAPP procrun.c \
	procgui.o \
	-lshlwapi \
	-lcomctl32 \
	-o $(BIN)/procrunw
