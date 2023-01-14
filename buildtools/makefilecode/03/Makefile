objs=main.o kbd.o command.o

edit : $(objs) 
	gcc -o edit $(objs)

main.o:defs.h

command.o:command.h

.PHONY:clean
clean:
	-rm edit $(objs)
