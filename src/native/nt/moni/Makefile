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
