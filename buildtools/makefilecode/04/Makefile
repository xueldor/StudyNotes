objs=main.o kbd.o command.o
edit : $(objs) 
	@gcc -o $@ $?

%.o:%.c
	@gcc -c -o $@ $<
	
clean:
	$(info "info txt")
	rm edit $(objs)

$(info $(SUFFIXES))
