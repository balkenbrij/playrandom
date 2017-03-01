####### Compiler, tools and options

CC	?=	gcc
CXX ?=  g++
ifeq ($(CC),)
CC	:=	gcc
endif
ifeq ($(CXX),)
CC	:=	g++
endif
INCPATH :=	.
CFLAGS	?=	-O2 -pipe -fstrict-aliasing -fstrict-overflow
CFLAGS	:=	$(CFLAGS) -Wall -Wextra -pedantic -DNO_DEBUG
LDLIBS	:=	-lbsd
LDFLAGS	:=	
CXXFLAGS:= $(CFLAGS)

INSTALL_PATH :=	/usr/local/bin

####### Files

BINARY	:=	playrandom
HEADERS :=
SOURCES :=	playrandom.c
OBJECTS :=	playrandom.o

####### Implicit rules

.cc.o:
	$(CXX) -c $(CXXFLAGS) $(FILESIZE_DEFS) -I$(INCPATH) -o $@ $<

.c.o:
	$(CC) -c $(CFLAGS) $(FILESIZE_DEFS) -I$(INCPATH) -o $@ $<

####### Build rules

all: $(BINARY)

$(BINARY): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $(BINARY) $(OBJECTS) $(LDLIBS)
	strip -s -R .note -R .comment --strip-unneeded $(BINARY)
	size $(BINARY)

clean:
	-rm -f core *.o
	-rm -f $(BINARY)

install:
	cp $(BINARY) $(INSTALL_PATH)

uninstall:
	rm $(INSTALL_PATH)/$(BINARY)

####### Dependencies

playrandom.o: playrandom.c

