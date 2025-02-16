OS:=$(shell uname -s)

ifeq ($(OS), Darwin)
    CC := clang
    CFLAGS := -g -lstdc++ -o3
endif

ifeq ($(OS), Linux)
    CC := c++
    CFLAGS := -g -o3
 endif

 vpath %.c src

 APPNAME=elfdump
 CFILES := elfdum.c
 OBJFILES := $(CFILES:.c=.o)

 all:$(OBJFILES)
    $(info "input file", $^)
    $(CC) -o buid/
