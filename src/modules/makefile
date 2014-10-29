CFLAGS := -shared -fPIC
SRC := $(shell ls *.c)
OBJ := $(SRC:%.c=lib%.so)

all: $(SRC)
	cc -o $(OBJ) $(CFLAGS) $(SRC)

.PHONY: test

test : 
	mv *.so ..

