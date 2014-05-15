include ../../config

SRC = $(wildcard *.cpp */*.cpp)
SRC_EXCLUDE = game/test.cpp misc/fibheap.cpp
CSRC += misc/snprintf.c

SRC := $(filter-out $(SRC_EXCLUDE), $(SRC))
OBJECTS = $(SRC:.cpp=.o)
OBJECTS += $(CSRC:.c=.o)

CC ?= gcc
CXX ?= g++

.PHONY: all freecnc-bin plugins depend clean clean-freecnc clean-plugins

all: freecnc-bin plugins

freecnc-bin: ../../freecnc

../../freecnc: $(OBJECTS) ../../config
	@test -s Makefile.depend || $(MAKE) depend
	$(CXX) $(FREECNC_LDFLAGS) -o ../../freecnc $(OBJECTS) $(FREECNC_LIBS)

plugins:
	@+$(MAKE) -C vfs/vfs_mix
	@+$(MAKE) -C vfs/vfs_tgz

depend:
	echo '' > Makefile.depend
	makedepend -fMakefile.depend -Y./include $(SRC) &> /dev/null
	test -e Makefile.depend.bak && rm -f Makefile.depend.bak


clean: clean-freecnc clean-plugins

clean-freecnc:
	\rm -f $(OBJECTS) ../../freecnc Makefile.depend

clean-plugins:
	@+$(MAKE) -C vfs/vfs_mix clean
	@+$(MAKE) -C vfs/vfs_tgz clean

.SUFFIXES: .cpp .c
.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $(<:.cpp=.o) $(FREECNC_CFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $(<:.c=.o) $(FREECNC_CFLAGS)

-include Makefile.depend
