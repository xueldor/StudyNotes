objs=main.o kbd.o command.o
edit : $(objs) 
	gcc -o edit $(objs)

main.o:main.c defs.h
	VPATH = src include
	gcc -c $< -Iinclude
kbd.o:kbd.c
	gcc -c $< -Iinclude
command.o:command.c command.h
	gcc -c $< -Iinclude
	
clean:
	rm edit main.o kbd.o command.o

