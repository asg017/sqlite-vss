

CGO_LDFLAGS="-L$(SQLITE_VSS_LIB_DIR)"

demo: demo.go
	CGO_LDFLAGS="$(CGO_LDFLAGS)" \
	go build -o $@
