
demo: demo.c
	gcc $< \
		-I../../dist/debug \
		-L../../dist/debug \
		-DSQLITE_CORE \
		-lsqlite_vector0 -lsqlite_vss0 -lfaiss_avx2 \
		-Wl,-undefined,dynamic_lookup \
		-lsqlite3 $(LDFLAGS) \
		-o $@
