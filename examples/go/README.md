# `sqlite-vss` in Go example

To build:

```
make demo
```

On Linux machines you'll need to link to many other libraries, which you can do with:

```
make demo CGO_LDFLAGS="-lm -latlas -lblas -llapack -lgomp -lstdc++"
```
