include ../../../../config

SRC = tgzfiles.cpp

OBJECTS = $(SRC:.cpp=.o)

.PHONY: clean depend

../../../../tgz.vfs: $(OBJECTS) ../../../../config
	@test -s Makefile.depend || $(MAKE) depend
	$(CXX) -o ../../../../tgz.vfs $(DEBUG_FLAGS) $(OBJECTS) $(TGZ_PLUGIN_LDFLAGS) $(TGZ_PLUGIN_LIBS)

clean:
	\rm -f $(OBJECTS) ../../../../tgz.vfs Makefile.depend

depend:
	echo '' > Makefile.depend
	makedepend -fMakefile.depend -Y../../../include $(SRC) &> /dev/null
	test -e Makefile.depend.bak && rm -f Makefile.depend.bak

.SUFFIXES: .cpp .o
.cpp.o:
	$(CXX) $(CXXFLAGS) $(TGZ_PLUGIN_CFLAGS) -c $< -o $(<:.cpp=.o)

-include Makefile.depend
