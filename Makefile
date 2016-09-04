####### Compiler, tools and options

CC	?=	gcc
ifeq ($(CC),)
CC	:=	gcc
endif
INCPATH :=	.
CFLAGS	?=	-O2
CFLAGS	:=	$(CFLAGS) -Wall -Wextra -pedantic -DNO_DEBUG
LDLIBS	:=	
LDFLAGS	:=	

INSTALL_PATH :=	/usr/local/bin

####### Files

BINARY	:=	playrandom
HEADERS :=
SOURCES :=	playrandom.c
OBJECTS :=	playrandom.o

####### Implicit rules

.SUFFIXES: .c

.c.o:
	$(CC) -c $(CFLAGS) $(FILESIZE_DEFS) -I$(INCPATH) -o $@ $<

####### Build rules

all: $(BINARY)

$(BINARY): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $(BINARY) $(OBJECTS) $(LDLIBS)
	strip -s $(BINARY)
	size $(BINARY)

clean:
	-rm -f core *.o
#	-rm -f $(BINARY)

install:
	cp $(BINARY) $(INSTALL_PATH)

uninstall:
	rm $(INSTALL_PATH)/$(BINARY)

####### Dependencies

playrandom.o: playrandom.c

