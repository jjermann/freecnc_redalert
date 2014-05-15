include ../../config

LOCALSRC = mixshell.cpp filename.cpp mixvfs.cpp blowfish.cpp ws-key.cpp
SRC = ../../client/src/misc/strcase.cpp
LOCALOBJ = $(LOCALSRC:.cpp=.o)
OBJ = $(SRC:.cpp=.o)

.PHONY: clean

../../mixshell: $(LOCALOBJ) $(OBJ)
	$(CXX) -o ../../mixshell $(LOCALOBJ) $(OBJ)

clean:
	\rm -rf $(LOCALOBJ) ../../mixshell

.SUFFIXES: .cpp .o
.cpp.o:
	$(CXX) $(CFLAGS) -c $< -o $(<:.cpp=.o) $(TOOLS_CFLAGS)
