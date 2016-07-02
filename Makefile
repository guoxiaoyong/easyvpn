CC=gcc
CFLAGS= -Wall -Werror -Wextra -fno-strict-aliasing -O2 -I. $(DW)

srcfiles := $(wildcard *.c *.h)

useful := $(srcfiles) Makefile evset rtset README
rmfiles := $(filter-out $(useful),$(wildcard *))

ifeq ($(D),DEBUG)
		DW := -DDEBUG
else
		DW :=
endif

all: easyvpn

easyvpn: main.o easyvpn.o debugmsg.o evpn_option.o aux.o


.PHONY: clean

clean:
	$(RM) $(rmfiles)
