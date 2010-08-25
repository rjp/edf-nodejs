# UNaXcess II Conferencing System
# (c) 1999 Michael Wood (mike@compsoc.man.ac.uk)
#
# Build file for sub directories (delete as appropriate)

include Makefile.inc

all: EDF/EDF.o EDF/EDFElement.o useful/useful.o useful/StackTrace.o

test: edf-to-json.o $(OBJS)
	$(LD) -o edf-to-json edf-to-json.o $(OBJS) $(LDFLAGS)

OBJS=EDF/EDF.o EDF/EDFElement.o useful/useful.o useful/StackTrace.o

edf-to-json.o: edf-to-json.cpp
EDF/EDF.o: EDF/EDF.cpp
EDF/EDFElement.o: EDF/EDFElement.cpp
useful/useful.o: useful/useful.cpp
useful/StackTrace.o: useful/StackTrace.cpp
