# `sqlite-vss` in C Example

To build:

```
make demo
```

On Linux machines you'll need to link to many other libraries, which you can do with:

```
make demo LDFLAGS="-lgomp -latlas -lblas -llapack -lm -lstdc++"
```
