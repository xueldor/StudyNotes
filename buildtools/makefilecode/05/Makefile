.PHONY:all

all: edit

edit: main.c libhello.a
	gcc main.c -I lib -L lib -l hello -o edit

libhello.a:
	cd lib && $(MAKE) 
	#或者
	#$(MAKE) -C lib
clean:
	rm edit $(objs) && $(MAKE) -C lib clean


