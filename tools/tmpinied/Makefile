include ../../config

LOCALSRC += tmpinied.cpp tmpgfx.cpp
SRC += ../../client/src/vfs/vfs.cpp
SRC += ../../client/src/vfs/vfsplugman.cpp ../../client/src/vfs/externalvfs.cpp
SRC += ../../client/src/misc/strcase.cpp ../../client/src/ui/logger.cpp
SRC += ../../client/src/misc/dllibrary.cpp ../../client/src/video/message.cpp
SRC += ../../client/src/ui/font.cpp ../../client/src/video/shpimage.cpp
SRC += ../../client/src/misc/inifile.cpp ../../client/src/video/imageproc.cpp
SRC += ../../client/src/misc/compression.cpp ../../client/src/misc/common.cpp

LOCALOBJS = $(LOCALSRC:.cpp=.o)
OBJECTS = $(SRC:.cpp=.o)

.PHONY: clean

../../tmpinied: $(LOCALOBJS) $(OBJECTS)
	$(CXX) -o ../../tmpinied -g $(LOCALOBJS) $(OBJECTS) $(FREECNC_LIBS) $(FREECNC_LDFLAGS)

clean:
	\rm -f $(LOCALOBJS) ../../tmpinied

.SUFFIXES: .cpp .o
.cpp.o:
	$(CXX) $(CFLAGS) -c $< -o $(<:.cpp=.o) $(TOOLS_CFLAGS)
