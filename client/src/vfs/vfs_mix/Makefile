include ../../../../config

SRC = blowfish.cpp mixvfs.cpp ws-key.cpp

OBJECTS = $(SRC:.cpp=.o)

.PHONY: clean depend

../../../../mix.vfs: $(OBJECTS) ../../../../config
	@test -s Makefile.depend || $(MAKE) depend
	$(CXX) -o ../../../../mix.vfs $(DEBUG_FLAGS) $(OBJECTS) $(MIX_PLUGIN_LIBS) $(MIX_PLUGIN_LDFLAGS)

clean:
	\rm -f $(OBJECTS) ../../../../mix.vfs Makefile.depend

depend:
	echo '' > Makefile.depend
	makedepend -fMakefile.depend -Y../../../include $(SRC) &> /dev/null
	test -e Makefile.depend.bak && rm -f Makefile.depend.bak

.SUFFIXES: .cpp .o
.cpp.o:
	$(CXX) $(CXXFLAGS) $(MIX_PLUGIN_CFLAGS) -c $< -o $(<:.cpp=.o)

-include Makefile.depend
