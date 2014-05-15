include ../../config

LOCALSRC = unmix.cpp filename.cpp mixvfs.cpp blowfish.cpp ws-key.cpp
SRC = ../../client/src/misc/strcase.cpp
LOCALOBJ = $(LOCALSRC:.cpp=.o)
OBJ = $(SRC:.cpp=.o)

.PHONY: clean

../../unmix: $(LOCALOBJ) $(OBJ)
	$(CXX) -o ../../unmix $(LOCALOBJ) $(OBJ)

clean:
	\rm -rf $(LOCALOBJ) ../../unmix

.SUFFIXES: .cpp .o
.cpp.o:
	$(CXX) $(CFLAGS) -c $< -o $(<:.cpp=.o) $(TOOLS_CFLAGS)
