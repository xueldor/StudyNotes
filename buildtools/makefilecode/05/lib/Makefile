.PHONY: all
all: libhello.a

libhello.a:hello.o
	ar rcs libhello.a hello.o

hello.o:hello.c hello.h
	gcc -c hello.c -o hello.o

clean:
	rm -rf *.a *.o
