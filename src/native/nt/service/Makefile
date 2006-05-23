INCLUDE=-I../lib -I ../../unix/native
../../../../dist/INSTSVC: instmain.c arguments.o debug.o
	gcc $(INCLUDE) instmain.c   -o ../../../../dist/INSTSVC arguments.o debug.o
arguments.o: ../../unix/native/arguments.c
	gcc -c $(INCLUDE) ../../unix/native/arguments.c
debug.o: ../../unix/native/debug.c
	gcc -c $(INCLUDE) ../../unix/native/debug.c
clean:
	rm -f arguments.o debug.o
