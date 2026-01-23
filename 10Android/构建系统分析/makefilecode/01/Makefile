edit : main.o kbd.o command.o 
	gcc -o edit main.o kbd.o command.o

main.o:main.c defs.h
	gcc -c main.c
kbd.o:kbd.c
	gcc -c kbd.c
command.o:command.c command.h
	gcc -c command.c
	
clean:
	rm edit main.o kbd.o command.o