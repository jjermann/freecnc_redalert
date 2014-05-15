include ../config

.PHONY: all clean

all:
	@+$(MAKE) -C audplay/
	@+$(MAKE) -C shpview/
	@+$(MAKE) -C tmpinied/
ifneq "Darwin" "$(shell uname -s)"
	@+$(MAKE) -C mixshell/
	@+$(MAKE) -C unmix/
endif
ifeq "1" "$(XMMS)"
	@+ $(MAKE) -C xmms-aud/
endif

clean:
	@+$(MAKE) -C audplay clean
	@+$(MAKE) -C shpview clean
	@+$(MAKE) -C tmpinied clean
ifneq "Darwin" "$(shell uname -s)"
	@+$(MAKE) -C mixshell clean
	@+$(MAKE) -C unmix clean
endif
ifeq "1" "$(XMMS)"
	@+$(MAKE) -C xmms-aud clean
endif
