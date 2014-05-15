include ../../config

LOCALSRC += minigfx.cpp shpview.cpp
SRC += ../../client/src/vfs/vfs.cpp ../../client/src/video/message.cpp
SRC += ../../client/src/vfs/vfsplugman.cpp ../../client/src/vfs/externalvfs.cpp
SRC += ../../client/src/misc/compression.cpp ../../client/src/misc/inifile.cpp
SRC += ../../client/src/misc/strcase.cpp ../../client/src/ui/logger.cpp
SRC += ../../client/src/misc/dllibrary.cpp ../../client/src/ui/font.cpp
SRC += ../../client/src/video/shpimage.cpp ../../client/src/video/imageproc.cpp
SRC += ../../client/src/misc/common.cpp

LOCALOBJS = $(LOCALSRC:.cpp=.o)
OBJECTS = $(SRC:.cpp=.o)

.PHONY: clean

../../shpview: $(LOCALOBJS) $(OBJECTS)
	$(CXX) -o ../../shpview -g $(LDFLAGS) $(LOCALOBJS) $(OBJECTS) $(FREECNC_LIBS) $(FREECNC_LDFLAGS)

clean:
	\rm -f $(LOCALOBJS) ../../shpview

.SUFFIXES: .cpp .o
.cpp.o:
	$(CXX) $(CFLAGS) -c $< -o $(<:.cpp=.o) $(TOOLS_CFLAGS)
