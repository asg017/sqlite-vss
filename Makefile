SHELL := /bin/bash
VERSION=$(shell cat VERSION)
CMAKE_VERSION=$(shell sed 's/-alpha//g' VERSION)

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

TARGET_LOADABLE_VECTOR=$(prefix)/debug/vector0.$(LOADABLE_EXTENSION)
TARGET_LOADABLE_VSS=$(prefix)/debug/vss0.$(LOADABLE_EXTENSION)
TARGET_LOADABLE=$(TARGET_LOADABLE_VECTOR) $(TARGET_LOADABLE_VSS)

TARGET_LOADABLE_RELEASE_VSS=$(prefix)/release/vss0.$(LOADABLE_EXTENSION)
TARGET_LOADABLE_RELEASE_VECTOR=$(prefix)/release/vector0.$(LOADABLE_EXTENSION)
TARGET_LOADABLE_RELEASE=$(TARGET_LOADABLE_RELEASE_VECTOR) $(TARGET_LOADABLE_RELEASE_VSS)

TARGET_WHEELS=$(prefix)/debug/wheels
TARGET_WHEELS_RELEASE=$(prefix)/release/wheels

INTERMEDIATE_PYPACKAGE_EXTENSION=python/sqlite_vss/sqlite_vss/

$(prefix):
	mkdir -p $(prefix)/debug
	mkdir -p $(prefix)/release

$(TARGET_LOADABLE): export SQLITE_VSS_CMAKE_VERSION = $(CMAKE_VERSION)
$(TARGET_LOADABLE_RELEASE): export SQLITE_VSS_CMAKE_VERSION = $(CMAKE_VERSION)

$(TARGET_LOADABLE): $(prefix) src/vss-extension.cpp src/vector-extension.cpp src/sqlite-vss.h.in 
	cmake -B build; make -C build
	cp build/vector0.$(LOADABLE_EXTENSION) $(TARGET_LOADABLE_VECTOR)
	cp build/vss0.$(LOADABLE_EXTENSION) $(TARGET_LOADABLE_VSS)

$(TARGET_LOADABLE_RELEASE): $(prefix)
	cmake -DCMAKE_BUILD_TYPE=Release -B build_release; make -C build_release
	cp build_release/vector0.$(LOADABLE_EXTENSION) $(TARGET_LOADABLE_RELEASE_VECTOR)
	cp build_release/vss0.$(LOADABLE_EXTENSION) $(TARGET_LOADABLE_RELEASE_VSS)


$(TARGET_WHEELS): $(prefix)
	mkdir -p $(TARGET_WHEELS)

$(TARGET_WHEELS_RELEASE): $(prefix)
	mkdir -p $(TARGET_WHEELS_RELEASE)


loadable: $(TARGET_LOADABLE)

loadable-release: $(TARGET_LOADABLE_RELEASE)


python: $(TARGET_WHEELS) $(TARGET_LOADABLE) python/sqlite_vss/setup.py python/sqlite_vss/sqlite_vss/__init__.py scripts/rename-wheels.py
	cp $(TARGET_LOADABLE_VECTOR) $(INTERMEDIATE_PYPACKAGE_EXTENSION) 
	cp $(TARGET_LOADABLE_VSS) $(INTERMEDIATE_PYPACKAGE_EXTENSION) 
	rm $(TARGET_WHEELS)/sqlite_vss* || true
	pip3 wheel python/sqlite_vss/ -w $(TARGET_WHEELS)
	python3 scripts/rename-wheels.py $(TARGET_WHEELS) $(RENAME_WHEELS_ARGS)
	echo "✅ generated python wheel"

python-release: $(TARGET_LOADABLE_RELEASE) $(TARGET_WHEELS_RELEASE) python/sqlite_vss/setup.py python/sqlite_vss/sqlite_vss/__init__.py scripts/rename-wheels.py
	cp $(TARGET_LOADABLE_RELEASE_VECTOR) $(INTERMEDIATE_PYPACKAGE_EXTENSION) 
	cp $(TARGET_LOADABLE_RELEASE_VSS) $(INTERMEDIATE_PYPACKAGE_EXTENSION) 
	rm $(TARGET_WHEELS_RELEASE)/sqlite_vss* || true
	pip3 wheel python/sqlite_vss/ -w $(TARGET_WHEELS_RELEASE)
	python3 scripts/rename-wheels.py $(TARGET_WHEELS_RELEASE) $(RENAME_WHEELS_ARGS)
	echo "✅ generated release python wheel"

python-versions: python/version.py.tmpl
	VERSION=$(VERSION) envsubst < python/version.py.tmpl > python/sqlite_vss/sqlite_vss/version.py
	echo "✅ generated python/sqlite_vss/sqlite_vss/version.py"

	VERSION=$(VERSION) envsubst < python/version.py.tmpl > python/datasette_sqlite_vss/datasette_sqlite_vss/version.py
	echo "✅ generated python/datasette_sqlite_vss/datasette_sqlite_vss/version.py"
	
datasette: $(TARGET_WHEELS) python/datasette_sqlite_vss/setup.py python/datasette_sqlite_vss/datasette_sqlite_vss/__init__.py
	rm $(TARGET_WHEELS)/datasette* || true
	pip3 wheel python/datasette_sqlite_vss/ --no-deps -w $(TARGET_WHEELS)

datasette-release: $(TARGET_WHEELS_RELEASE) python/datasette_sqlite_vss/setup.py python/datasette_sqlite_vss/datasette_sqlite_vss/__init__.py
	rm $(TARGET_WHEELS_RELEASE)/datasette* || true
	pip3 wheel python/datasette_sqlite_vss/ --no-deps -w $(TARGET_WHEELS_RELEASE)

npm: VERSION npm/platform-package.README.md.tmpl npm/platform-package.package.json.tmpl npm/sqlite-vss/package.json.tmpl scripts/npm_generate_platform_packages.sh
	scripts/npm_generate_platform_packages.sh

deno: VERSION deno/deno.json.tmpl
	scripts/deno_generate_package.sh

version:
	make python-versions
	make python
	make npm
	make deno

test-loadable:
	$(PYTHON) tests/test-loadable.py

test-python:
	$(PYTHON) tests/test-python.py

test-npm:
	node npm/sqlite-vss/test.js

test-deno:
	deno task --config deno/deno.json test

test:
	make test-loadable
	make test-python
	make test-npm
	make test-deno


test-loadable-3.41.0:
	LD_LIBRARY_PATH=vendor/sqlite-snapshot-202301161813/.libs/  make test 

.PHONY: clean test test-3.41.0 loadable \
	python python-release python-versions datasette npm deno version