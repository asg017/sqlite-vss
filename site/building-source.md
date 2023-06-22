# Building `sqlite-vss` from source

If there isn't a prebuilt `sqlite-vss` build for your operating system or CPU architecture, you can try building `sqlite-vss` yourself. It'll require a C++ compiler, cmake, and possibly a few other libraries to build correctly.

Below are the general steps to build `sqlite-vss`. Your operating system may require a few more libraries or setup instructions, so some tips are listed below under [Platform-specific compiling tips](#platform-specific-compiling-tips).

## Instructions

To start, clone this repository and its submodules.

```bash
git clone --recurse-submodules https://github.com/asg017/sqlite-vss.git
cd sqlite-vss
```

Next, you'll need to build a vendored version of SQLite under `vendor/sqlite`. To retrieve the SQLite amalgammation, run the `./vendor/get_sqlite.sh` script:

```bash
./vendor/get_sqlite.sh
```

Then navigate to the newly built `vendor/sqlite` directory and build the SQLite library.

```bash
cd vendor/sqlite
./configure && make
```

Now that all dependencies are downloaded and configured, you can build the `sqlite-vss` extension! Run either `make loadable` to build a loadable SQLite extension.

```bash
# Build a debug version of `sqlite-vss`.
# Faster to compile, slower at runtime
make loadable
```

After it finishes, you'll find debug version of `vector0` and `vss0` inside `dist/debug`, with file extensions `.dylib`, `.so`, or `.dll`, depending on your operating system.

To compile static library files of `sqlite-vss` and `sqlite-vector`, run `make static`.

```bash
# Build a debug static archive files of `sqlite-vss`, ".a" and ".h" files
make static
```

After it completes, you'll find `.a` and `.h` files inside `dist/debug`, which can be used to statically link `sqlite-vss` into other projects. The [Rust bindings](./rust) and [Go bindings](./go) uses this approach.

```
-I./dist/debug -lsqlite_vector0 -lsqlite_vss0 -llfaiss_avx2
```

For better runtime performance and smaller binaries, consider instead `make loadable-release` and `make static-release`.

```bash

# Build a release version of `sqlite-vss`.
# Slower to compile, but faster at runtime
make loadable-release

make static-release
```

You'll file the optimized loadable and static files under `dist/release`.

## Platform-specific compiling tips

### MacOS (x86_64)

On Macs, you may need to install and use `llvm` and `libomp` for compilation. It can be install with brew:

```bash
brew install llvm libomp
```

Additionally, if you see other cryptic compiling errors, you may need to explicitly state to use the `llvm` compilers, with flags like so:

```bash
export CC=/usr/local/opt/llvm/bin/clang
export CXX=/usr/local/opt/llvm/bin/clang++
export LDFLAGS="-L/usr/local/opt/llvm/lib"
export CPPFLAGS="-I/usr/local/opt/llvm/include"
```

If you come across any problems, please file an issue!

### MacOS (M1/M2 ARM)

For Mac M1/M2 computers, you'll need `llvm` and `libomp`:

```bash
brew install llvm libomp
```

You will also likely need the following compiler flags:

```bash
export CC="/opt/homebrew/opt/llvm/bin/clang"
export CXX="/opt/homebrew/opt/llvm/bin/clang++"
export LDFLAGS="-L/opt/homebrew/opt/libomp/lib"
export CPPFLAGS="-I/opt/homebrew/opt/libomp/include"
```

Note that these are _slightly_ different than the ones above, as `LDFLAGS`/`CPPFLAGS` are referencing libomp instead.

### Linux (x86_64)

You most likely will need to install the following libraries before compiling:

```bash
sudo apt-get update
sudo apt-get install libgomp1 libatlas-base-dev liblapack-dev libsqlite3-dev
```

Explainations for these packages:

- `libgomp1`: OpenMP implementation, for multi-threading. Required by Faiss
- `libatlas-base-dev`: Provides a BLAS implementation. Required by Faiss
- `liblapack-dev`: Provides an LAPACK implementation. Required by Faiss
- `libsqlite3-dev`: Provides SQLite headers/static files for dev purposes.
