# UNaXcess II Conferencing System
# (c) 1998 Michael Wood (mike@compsoc.man.ac.uk)
#
# Makefile.inc: Common build options
# 
# - To reduce warning output remove -Wall
# - To turn of SSL remove CCSECURE and LDSECURE


CC=g++
OSHACKS=
INCCCFLAGS=-g -Wall -O2 -DUNIX $(OSHACKS) -I.. -I.
CCFLAGS=$(INCCCFLAGS)

LD=$(CXX) # g++
LDSECURE= 
LDFLAGS=$(LDSECURE)
LIBS=-lncurses 

.SUFFIXES:$(SUFFIXES) .cpp .cc .c

.cpp.o:
	$(CC) $(CCFLAGS) -c $< -o $(@)

.cc.o:
	$(CC) $(CCFLAGS) -c $< -o $(@)

.c.o:
	$(CC) $(CCFLAGS) -c $< -o $(@)
