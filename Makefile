#
#  Establish the compilers

CC=gcc
CXX=g++
CXXLD=g++
CCLD=gcc

DAQLIB_DIR=$(DAQHOME)/lib

CXXFLAGS=-Wall -O3 -g -I. -I$(DAQHOME)/include -pthread -fPIC -I/mnt/analysis/e21069/JH/include
CCFLAGS=$(CXXFLAGS)

ifdef LEGACY_MODE
	CXXFLAGS+= -DLEGACY_MODE
endif

LDFLAGS=-Wl,-rpath=$(PWD)

ROOTCFLAGS   := $(shell ${ROOTSYS}/bin/root-config --cflags)
ROOTGLIBS    := $(shell ${ROOTSYS}/bin/root-config --glibs)
ROOTLDFLAGS  := $(shell ${ROOTSYS}/bin/root-config --ldflags)

LIBS  = -lm $(ROOTGLIBS) -L$(DAQLIB_DIR) -L$(shell pwd) -lddaschannel

CXXFLAGS+=$(ROOTCFLAGS)
O_FILES = eventBuilder.o

all: libddaschannel.so eventBuilder

eventBuilder: $(O_FILES) libddaschannel.so
	$(CXXLD) -o eventBuilder $(O_FILES) $(LDFLAGS) $(LIBS) 

ddaschannelDictionary.cpp: ddaschannel.h DDASEvent.h LinkDef.h
	rm -f ddaschannelDictionary.cpp ddaschannelDictionary.h
	$(ROOTSYS)/bin/rootcint -f $@ -c -p ddaschannel.h DDASEvent.h LinkDef.h 

ddaschannelDictionary.o: ddaschannelDictionary.cpp
	$(CXX) -c -o $@ $(CXXFLAGS) $^

libddaschannel.so: ddaschannelDictionary.o ddaschannel.o DDASEvent.o
	$(CXX) -shared -Wl,-soname,$@ -o $@ $(ROOTGLIBS) $(CXXFLAGS) $^

depend:
	makedepend $(CXXFLAGS) *.cpp *.c

clean:
	rm -f *.o eventBuilder libddaschannel.so ddaschannelDictionary*

print_env:
	@echo $(ROOTSYS)
	@echo $(ROOTCFLAGS)
	@echo $(ROOTGLIBS)
	@echo $(ROOTLDFLAGS)

