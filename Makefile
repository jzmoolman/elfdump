OS:=$(shell uname -s)

ifeq ($(OS), Darwin)
	CC := clang
	CFLAGS := -g -lstdc++ -O3
endif

ifeq ($(OS), Linux)
	CC := c++
	CFLAGS := -g -O3
endif

vpath %.c src

APPNAME=elfdump
CFILES := elfdump.c
OBJFILES := $(CFILES:.c=.o)

all:$(OBJFILES)
	$(info "input file", $^)
	$(CC) -o build/$(APPNAME) $(CFLAGS) $^
	@echo "Link => $a"

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

.PHONY: clean
clean:
	rm *.o
	rm $(APPNAME)

run:
	./build/$(APPNAME)

