all: bookorder

debug:
	make DEBUG=FALSE

bookorder:
	gcc -Wall -pthread -o bookorder bookorder.c -lpthread

ifeq ($(DEBUG), TRUE)
CCFLAGS += -g
endif

clean:
	rm -f bookorder

