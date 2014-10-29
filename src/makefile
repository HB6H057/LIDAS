MAINSRC := main.c ghost.c config.c heap.c http_parser.c listener.c log.c manager.c memory.c \
           proxy.c queue.c queue_chain.c read_event.c response.c snd.c thread.c timer.c
MAINFLAGS:= -lxml2 -g

all: $(MAINSRC)
	cc -o lida-main $(MAINSRC) $(MAINFLAGS)
	
.PHONY: clean debug

clean : 
	rm *.o
debug : 
	mv lida-main ..
