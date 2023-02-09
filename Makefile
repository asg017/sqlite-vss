SHELL := /bin/bash

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

ifdef python
PYTHON=$(python)
else
PYTHON=python3
endif

prefix=dist

TARGET_LOADABLE=$(prefix)/debug/vss0.$(LOADABLE_EXTENSION)
TARGET_LOADABLE_RELEASE=$(prefix)/release/vss0.$(LOADABLE_EXTENSION)

TARGET_LOADABLE_DEP=$(prefix)/debug/vector0.$(LOADABLE_EXTENSION)
TARGET_LOADABLE_DEP_RELEASE=$(prefix)/release/vector0.$(LOADABLE_EXTENSION)

TARGET_WHEELS=$(prefix)/debug/wheels
TARGET_WHEELS_RELEASE=$(prefix)/release/wheels

INTERMEDIATE_PYPACKAGE_EXTENSION=python/sqlite_vss/sqlite_vss/vss0.$(LOADABLE_EXTENSION)

$(prefix):
	mkdir -p $(prefix)/debug
	mkdir -p $(prefix)/release

$(TARGET_LOADABLE): $(prefix) src/extension.cpp
	cmake -B build; make -C build
	cp build/vss0.$(LOADABLE_EXTENSION) $@

$(TARGET_LOADABLE_RELEASE): $(prefix)
	cmake -DCMAKE_BUILD_TYPE=Release -B build_release; make -C build_release
	cp build_release/vss0.$(LOADABLE_EXTENSION) $@

$(TARGET_LOADABLE_DEP): $(prefix)
	(cd vendor/sqlite-vector; make loadable);
	cp vendor/sqlite-vector/dist/debug/vector0.$(LOADABLE_EXTENSION) $@

$(TARGET_LOADABLE_DEP_RELEASE): $(prefix)
	(cd vendor/sqlite-vector; make loadable-release);
	cp vendor/sqlite-vector/dist/release/vector0.$(LOADABLE_EXTENSION) $@

$(TARGET_WHEELS): $(prefix)
	mkdir -p $(TARGET_WHEELS)

$(TARGET_WHEELS_RELEASE): $(prefix)
	mkdir -p $(TARGET_WHEELS_RELEASE)


loadable: $(TARGET_LOADABLE) $(TARGET_LOADABLE_DEP) 

loadable-release: $(TARGET_LOADABLE_RELEASE) $(TARGET_LOADABLE_DEP_RELEASE)


python: $(TARGET_WHEELS) $(TARGET_LOADABLE) python/sqlite_vss/setup.py python/sqlite_vss/sqlite_vss/__init__.py .github/workflows/rename-wheels.py
	cp $(TARGET_LOADABLE) $(INTERMEDIATE_PYPACKAGE_EXTENSION) 
	rm $(TARGET_WHEELS)/sqlite_vss* || true
	pip3 wheel python/sqlite_vss/ -w $(TARGET_WHEELS)
	python3 .github/workflows/rename-wheels.py $(TARGET_WHEELS) $(RENAME_WHEELS_ARGS)

python-release: $(TARGET_LOADABLE_RELEASE) $(TARGET_WHEELS_RELEASE) python/sqlite_vss/setup.py python/sqlite_vss/sqlite_vss/__init__.py .github/workflows/rename-wheels.py
	cp $(TARGET_LOADABLE_RELEASE)  $(INTERMEDIATE_PYPACKAGE_EXTENSION) 
	rm $(TARGET_WHEELS_RELEASE)/sqlite_vss* || true
	pip3 wheel python/sqlite_vss/ -w $(TARGET_WHEELS_RELEASE)
	python3 .github/workflows/rename-wheels.py $(TARGET_WHEELS_RELEASE) $(RENAME_WHEELS_ARGS)

datasette: $(TARGET_WHEELS) python/datasette_sqlite_vss/setup.py python/datasette_sqlite_vss/datasette_sqlite_vss/__init__.py
	rm $(TARGET_WHEELS)/datasette* || true
	pip3 wheel python/datasette_sqlite_vss/ --no-deps -w $(TARGET_WHEELS)

datasette-release: $(TARGET_WHEELS_RELEASE) python/datasette_sqlite_vss/setup.py python/datasette_sqlite_vss/datasette_sqlite_vss/__init__.py
	rm $(TARGET_WHEELS_RELEASE)/datasette* || true
	pip3 wheel python/datasette_sqlite_vss/ --no-deps -w $(TARGET_WHEELS_RELEASE)


test-loadable:
	$(PYTHON) tests/test-loadable.py

test-python:
	$(PYTHON) tests/test-python.py

test:
	make test-loadable
	#make test-python


test-loadable-3.41.0:
	LD_LIBRARY_PATH=vendor/sqlite-snapshot-202301161813/.libs/  make test 

.PHONY: clean test test-3.41.0 loadable