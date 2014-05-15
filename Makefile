include setup/test_base
include setup/test_data

# MSYS's uname returns the OS as part of -s, e.g. MINGW32_NT-4.0
# So we translate this into something nicer.
SYSTEM=$(shell uname -s | sed -e 's/MINGW.*/Mingw/g')
SYSFILES=$(wildcard setup/$(SYSTEM)/config*)

.PHONY: default all client client-depend tools reconfig clean clean-client clean-tools clean-config help

default: client

all: client tools

client: config
	@+$(MAKE) -C client

client-depend:
	@test -s config || touch config
	@+$(MAKE) -C client depend
	@+$(MAKE) -C client/src/vfs/vfs_mix depend
	@+$(MAKE) -C client/src/vfs/vfs_tgz depend
	@test -e config && (test -s config || rm config)

tools: config
	@+$(MAKE) -C tools

config: setup/base/* $(SYSFILES)
	$(foreach test, $(TESTS), $(call TEST_template, $(test)))
	@echo Trying to use plugin creation rules for $(SYSTEM)
	@cat -- setup/base/config* > config
	@cp -- $(SYSFILES) . || cp -- setup/Linux/config* .
	@cat config_* >> config
	@rm -f config_*

reconfig:
	-rm -f config
	@$(MAKE) config

clean: clean-client clean-tools clean-config
	
clean-client:
	@test -s config || touch config
	@+$(MAKE) -C client clean
	@test -e config && (test -s config || rm -f config)

clean-tools:
	@test -s config || touch config
	@+$(MAKE) -C tools clean
	@test -e config && (test -s config || rm -f config)

clean-config:
	-rm -f config	

help:
	@echo "Build targets:"
	@echo "  client (default) - Compiles freecnc"
	@echo "  tools            - Compiles freecnc misc tools"
	@echo "  all              - Compiles client aswell as the tools"
	@echo
	@echo "Configuration targets:"
	@echo "  config           - Generates make config (auto)."
	@echo "  reconfig         - Regenerates config (this should not be needed)"
	@echo
	@echo "Cleaning targets:"
	@echo "  clean            - All of the clean targets combined"
	@echo "  clean-client     - Cleans up client"
	@echo "  clean-tools      - Cleans up tools"
	@echo "  clean-config     - Removes make config files"
	@echo
	@echo "Client specific targets:"
	@echo "  client-depend    - Regenerates client dependency files (auto)"
