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

Now that all dependencies are downloaded and configured, you can build the `sqlite-vss` extension! Run either `make loadable` or `make loadable-release` to build a loadable SQLite extension.

```bash
# Build a debug version of `sqlite-vss`.
# Faster to compile, slower at runtime
make loadable

# Build a release version of `sqlite-vss`.
# Slower to compile, but faster at runtime
make loadable-release
```

If you ran `make loadable`, then under `dist/debug` you'll find debug version of `vector0` and `vss0`, with file extensions `.dylib`, `.so`, or `.dll`, depending on your operating system. If you ran `make loadable-release`, you'll find optimized version of `vector0` and `vss0` under `dist/release`.

## Platform-specific compiling tips

### MacOS (x86_64)

On Macs, you may need to install and use `llvm` for compilation. It can be install with brew:

```bash
brew install llvm
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

I haven't tried compiling `sqlite-vss` on a M1 Mac yet, but others have reported success. See the above instructions if you have problems, or file an issue.

### Linux (x86_64)

You most likely will need to install the following libraries before compiling:

```bash
sudo apt-get update
sudo apt-get install libgomp1 libatlas-base-dev liblapack-dev
```
