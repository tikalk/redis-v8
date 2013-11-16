# Top level makefile, the real shit is at src/Makefile

DEPENDENCY_TARGETS=v8 hiredis linenoise lua

default: all

.DEFAULT:
	cd lib && $(MAKE) $@

v8:
	cd lib/deps && $(MAKE) $@

deps:
	cd lib/deps && $(MAKE) $(DEPENDENCY_TARGETS)

install:
	cd lib && $(MAKE) $@

.PHONY: install
