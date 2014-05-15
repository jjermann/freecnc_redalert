include ../../config

LOCALSRC += audplay.cpp stubs.cpp soundengine.cpp
SRC += ../../client/src/vfs/vfs.cpp
SRC += ../../client/src/audio/sound.cpp
SRC += ../../client/src/vfs/vfsplugman.cpp ../../client/src/vfs/externalvfs.cpp
SRC += ../../client/src/misc/compression.cpp ../../client/src/misc/inifile.cpp
SRC += ../../client/src/misc/strcase.cpp ../../client/src/ui/logger.cpp
SRC += ../../client/src/misc/dllibrary.cpp ../../client/src/misc/common.cpp

LOCALOBJS = $(LOCALSRC:.cpp=.o)
OBJECTS = $(SRC:.cpp=.o)

.PHONY: clean

../../audplay: $(LOCALOBJS) $(OBJECTS)
	$(CXX) -o ../../audplay -g $(LOCALOBJS) $(OBJECTS) $(FREECNC_LIBS) $(FREECNC_LDFLAGS)

clean:
	\rm -f $(LOCALOBJS) ../../audplay

.SUFFIXES: .cpp .o
.cpp.o:
	$(CXX) $(CFLAGS) -c $< -o $(<:.cpp=.o) $(TOOLS_CFLAGS)
