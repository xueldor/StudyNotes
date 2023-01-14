objs=main.o kbd.o command.o
edit : $(objs) 
	gcc -o edit $(objs)

main.o:main.c defs.h
	gcc -c main.c
kbd.o:kbd.c
	gcc -c kbd.c
command.o:command.c command.h
	gcc -c command.c
	
clean:
	rm edit main.o kbd.o command.o
