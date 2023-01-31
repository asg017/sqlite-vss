ifeq ($(shell uname -s),Darwin)
CONFIG_DARWIN=y
else ifeq ($(OS),Windows_NT)
CONFIG_WINDOWS=y
else
CONFIG_LINUX=y
endif

LIBRARY_PREFIX=lib
ifdef CONFIG_DARWIN
LOADABLE_EXTENSION=dylib
endif

ifdef CONFIG_LINUX
LOADABLE_EXTENSION=so
endif


ifdef CONFIG_WINDOWS
LOADABLE_EXTENSION=dll
LIBRARY_PREFIX=
endif

prefix=dist

loadable:
	cmake -B build; make -C build

loadable-release: 
	cmake -DCMAKE_BUILD_TYPE=Release -B build_release; make -C build_release

test:
	python3 tests/test-loadable.py

test-3.41.0:
	LD_LIBRARY_PATH=vendor/sqlite-snapshot-202301161813/.libs/  make test 

.PHONY: clean test test-3.41.0 loadable