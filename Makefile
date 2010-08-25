# UNaXcess II Conferencing System
# (c) 1999 Michael Wood (mike@compsoc.man.ac.uk)
#
# Build file for sub directories (delete as appropriate)

include Makefile.inc

all: EDF/EDF.o EDF/EDFElement.o useful/useful.o useful/StackTrace.o

EDF/EDF.o: EDF/EDF.cpp
EDF/EDFElement.o: EDF/EDFElement.cpp
useful/useful.o: useful/useful.cpp
useful/StackTrace.o: useful/StackTrace.cpp
