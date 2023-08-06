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

ifdef IS_MACOS_ARM
RENAME_WHEELS_ARGS=--is-macos-arm
else
RENAME_WHEELS_ARGS=
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

TARGET_STATIC_VECTOR=$(prefix)/debug/libsqlite_vector0.a
TARGET_STATIC_VECTOR_H=$(prefix)/debug/sqlite-vector.h
TARGET_STATIC_VSS=$(prefix)/debug/libsqlite_vss0.a
TARGET_STATIC_VSS_H=$(prefix)/debug/sqlite-vss.h
TARGET_STATIC_FAISS_AVX2=$(prefix)/debug/libfaiss_avx2.a
TARGET_STATIC=$(TARGET_STATIC_VECTOR) $(TARGET_STATIC_VSS) $(TARGET_STATIC_FAISS_AVX2)

TARGET_LOADABLE_RELEASE_VSS=$(prefix)/release/vss0.$(LOADABLE_EXTENSION)
TARGET_LOADABLE_RELEASE_VECTOR=$(prefix)/release/vector0.$(LOADABLE_EXTENSION)
TARGET_LOADABLE_RELEASE=$(TARGET_LOADABLE_RELEASE_VECTOR) $(TARGET_LOADABLE_RELEASE_VSS)

TARGET_STATIC_RELEASE_VECTOR=$(prefix)/release/libsqlite_vector0.a
TARGET_STATIC_RELEASE_VECTOR_H=$(prefix)/release/sqlite-vector.h
TARGET_STATIC_RELEASE_VSS=$(prefix)/release/libsqlite_vss0.a
TARGET_STATIC_RELEASE_VSS_H=$(prefix)/release/sqlite-vss.h
TARGET_STATIC_RELEASE_FAISS_AVX2=$(prefix)/release/libfaiss_avx2.a
TARGET_STATIC_RELEASE=$(TARGET_STATIC_RELEASE_VECTOR) $(TARGET_STATIC_RELEASE_VSS) $(TARGET_STATIC_RELEASE_FAISS_AVX2)

TARGET_WHEELS=$(prefix)/debug/wheels
TARGET_WHEELS_RELEASE=$(prefix)/release/wheels

INTERMEDIATE_PYPACKAGE_EXTENSION=bindings/python/sqlite_vss/

$(prefix):
	mkdir -p $(prefix)/debug
	mkdir -p $(prefix)/release

$(TARGET_LOADABLE): export SQLITE_VSS_CMAKE_VERSION = $(CMAKE_VERSION)
$(TARGET_LOADABLE_RELEASE): export SQLITE_VSS_CMAKE_VERSION = $(CMAKE_VERSION)

$(TARGET_LOADABLE): $(prefix) src/sqlite-vss.cpp src/sqlite-vector.cpp src/sqlite-vss.h.in
	cmake -B build; make -C build
	cp build/vector0.$(LOADABLE_EXTENSION) $(TARGET_LOADABLE_VECTOR)
	cp build/vss0.$(LOADABLE_EXTENSION) $(TARGET_LOADABLE_VSS)

$(TARGET_LOADABLE_RELEASE): $(prefix) src/sqlite-vss.cpp src/sqlite-vector.cpp src/sqlite-vss.h.in
	cmake -DCMAKE_BUILD_TYPE=Release -B build_release; make -C build_release
	cp build_release/vector0.$(LOADABLE_EXTENSION) $(TARGET_LOADABLE_RELEASE_VECTOR)
	cp build_release/vss0.$(LOADABLE_EXTENSION) $(TARGET_LOADABLE_RELEASE_VSS)

$(TARGET_STATIC) $(TARGET_STATIC_VECTOR_H) $(TARGET_STATIC_VSS_H): export SQLITE_VSS_CMAKE_VERSION = $(CMAKE_VERSION)
$(TARGET_STATIC_RELEASE) $(TARGET_STATIC_RELEASE_VECTOR_H) $(TARGET_STATIC_RELEASE_VSS_H): export SQLITE_VSS_CMAKE_VERSION = $(CMAKE_VERSION)

$(TARGET_STATIC) $(TARGET_STATIC_VECTOR_H) $(TARGET_STATIC_VSS_H): $(prefix) VERSION src/sqlite-vss.cpp src/sqlite-vector.cpp src/sqlite-vss.h.in
	cmake -B build; make -C build
	cp build/libsqlite_vector0.a $(TARGET_STATIC_VECTOR)
	cp build/libsqlite_vss0.a $(TARGET_STATIC_VSS)
	cp build/vendor/faiss/faiss/libfaiss_avx2.a $(TARGET_STATIC_FAISS_AVX2)
	cp build/sqlite-vector.h $(TARGET_STATIC_VECTOR_H)
	cp build/sqlite-vss.h $(TARGET_STATIC_VSS_H)

$(TARGET_STATIC_RELEASE) $(TARGET_STATIC_RELEASE_VECTOR_H) $(TARGET_STATIC_RELEASE_VSS_H): $(prefix) VERSION src/sqlite-vss.cpp src/sqlite-vector.cpp src/sqlite-vss.h.in src/sqlite-vector.h.in
	cmake -DCMAKE_BUILD_TYPE=Release -B build_release; make -C build_release
	cp build_release/libsqlite_vector0.a $(TARGET_STATIC_RELEASE_VECTOR)
	cp build_release/libsqlite_vss0.a $(TARGET_STATIC_RELEASE_VSS)
	cp build_release/vendor/faiss/faiss/libfaiss_avx2.a $(TARGET_STATIC_RELEASE_FAISS_AVX2)
	cp build_release/sqlite-vector.h $(TARGET_STATIC_RELEASE_VECTOR_H)
	cp build_release/sqlite-vss.h $(TARGET_STATIC_RELEASE_VSS_H)


$(TARGET_WHEELS): $(prefix)
	mkdir -p $(TARGET_WHEELS)

$(TARGET_WHEELS_RELEASE): $(prefix)
	mkdir -p $(TARGET_WHEELS_RELEASE)


loadable: $(TARGET_LOADABLE)
loadable-release: $(TARGET_LOADABLE_RELEASE)

static: $(TARGET_STATIC)
static-release: $(TARGET_STATIC_RELEASE)


python: $(TARGET_WHEELS) $(TARGET_LOADABLE) bindings/python/setup.py bindings/python/sqlite_vss/__init__.py scripts/rename-wheels.py
	cp $(TARGET_LOADABLE_VECTOR) $(INTERMEDIATE_PYPACKAGE_EXTENSION)
	cp $(TARGET_LOADABLE_VSS) $(INTERMEDIATE_PYPACKAGE_EXTENSION)
	rm $(TARGET_WHEELS)/sqlite_vss* || true
	pip3 wheel bindings/python/ -w $(TARGET_WHEELS)
	python3 scripts/rename-wheels.py $(TARGET_WHEELS) $(RENAME_WHEELS_ARGS)
	echo "✅ generated python wheel"

python-release: $(TARGET_LOADABLE_RELEASE) $(TARGET_WHEELS_RELEASE) bindings/python/setup.py bindings/python/sqlite_vss/__init__.py scripts/rename-wheels.py
	cp $(TARGET_LOADABLE_RELEASE_VECTOR) $(INTERMEDIATE_PYPACKAGE_EXTENSION)
	cp $(TARGET_LOADABLE_RELEASE_VSS) $(INTERMEDIATE_PYPACKAGE_EXTENSION)
	rm $(TARGET_WHEELS_RELEASE)/sqlite_vss* || true
	pip3 wheel bindings/python/ -w $(TARGET_WHEELS_RELEASE)
	python3 scripts/rename-wheels.py $(TARGET_WHEELS_RELEASE) $(RENAME_WHEELS_ARGS)
	echo "✅ generated release python wheel"

python-versions: bindings/python/version.py.tmpl bindings/datasette/version.py.tmpl
	VERSION=$(VERSION) envsubst < bindings/python/version.py.tmpl > bindings/python/sqlite_vss/version.py
	echo "✅ generated bindings/python/sqlite_vss/version.py"

	VERSION=$(VERSION) envsubst < bindings/datasette/version.py.tmpl > bindings/datasette/datasette_sqlite_vss/version.py
	echo "✅ generated bindings/datasette/datasette_sqlite_vss/version.py"

datasette: $(TARGET_WHEELS) bindings/datasette/setup.py bindings/datasette/datasette_sqlite_vss/__init__.py
	rm $(TARGET_WHEELS)/datasette* || true
	pip3 wheel bindings/datasette/ --no-deps -w $(TARGET_WHEELS)

datasette-release: $(TARGET_WHEELS_RELEASE) bindings/datasette/setup.py bindings/datasette/datasette_sqlite_vss/__init__.py
	rm $(TARGET_WHEELS_RELEASE)/datasette* || true
	pip3 wheel bindings/datasette/ --no-deps -w $(TARGET_WHEELS_RELEASE)

bindings/sqlite-utils/pyproject.toml: bindings/sqlite-utils/pyproject.toml.tmpl VERSION
	VERSION=$(VERSION) envsubst < $< > $@
	echo "✅ generated $@"

bindings/sqlite-utils/sqlite_utils_sqlite_vss/version.py: bindings/sqlite-utils/sqlite_utils_sqlite_vss/version.py.tmpl VERSION
	VERSION=$(VERSION) envsubst < $< > $@
	echo "✅ generated $@"

sqlite-utils: $(TARGET_WHEELS) bindings/sqlite-utils/pyproject.toml bindings/sqlite-utils/sqlite_utils_sqlite_vss/version.py
	python3 -m build bindings/sqlite-utils -w -o $(TARGET_WHEELS)

sqlite-utils-release: $(TARGET_WHEELS) bindings/sqlite-utils/pyproject.toml bindings/sqlite-utils/sqlite_utils_sqlite_vss/version.py
	python3 -m build bindings/sqlite-utils -w -o $(TARGET_WHEELS_RELEASE)

npm: VERSION bindings/node/platform-package.README.md.tmpl bindings/node/platform-package.package.json.tmpl bindings/node/sqlite-vss/package.json.tmpl scripts/npm_generate_platform_packages.sh
	scripts/npm_generate_platform_packages.sh

deno: VERSION bindings/deno/deno.json.tmpl
	scripts/deno_generate_package.sh

bindings/ruby/lib/version.rb: bindings/ruby/lib/version.rb.tmpl VERSION
	VERSION=$(VERSION) envsubst < $< > $@

bindings/rust/Cargo.toml: bindings/rust/Cargo.toml.tmpl VERSION
	VERSION=$(VERSION) envsubst < $< > $@

rust: bindings/rust/Cargo.toml

bindings/go/vector/sqlite-vector.h: $(TARGET_STATIC_VECTOR_H)
	cp $< $@

bindings/go/vss/sqlite-vss.h: $(TARGET_STATIC_VSS_H)
	cp $< $@

go:
	make bindings/go/vector/sqlite-vector.h
	make bindings/go/vss/sqlite-vss.h

bindings/elixir/VERSION: VERSION
	cp $< $@

elixir: bindings/elixir/VERSION

version:
	make python-versions
	make python
	make bindings/sqlite-utils/pyproject.toml bindings/sqlite-utils/sqlite_utils_sqlite_vss/version.py
	make npm
	make deno
	make bindings/ruby/lib/version.rb
	make bindings/rust/Cargo.toml
	make go
	make rust
	make elixir

test-loadable:
	$(PYTHON) tests/test-loadable.py

test-python:
	$(PYTHON) tests/test-python.py

test-npm:
	node bindings/node/sqlite-vss/test.js

test-deno:
	deno task --config bindings/deno/deno.json test

test:
	make test-loadable
	make test-python
	make test-npm
	make test-deno

patch-openmp:
	patch --forward --reject-file=/dev/null --quiet -i scripts/openmp_static.patch vendor/faiss/faiss/CMakeLists.txt 2> /dev/null || true

patch-openmp-undo:
	patch -R vendor/faiss/faiss/CMakeLists.txt < scripts/openmp_static.patch

test-loadable-3.41.0:
	LD_LIBRARY_PATH=vendor/sqlite-snapshot-202301161813/.libs/  make test

site-dev:
	npm --prefix site run dev

site-build:
	npm --prefix site run build

publish-release:
	./scripts/publish_release.sh

.PHONY: clean test test-3.41.0 \
	loadable loadable-release static static-release \
	publish-release \
	patch-openmp patch-openmp-undo \
	python python-release python-versions datasette sqlite-utils sqlite-utils-release elixir npm deno go rust version
