#		-L/usr/local/opt/sqlite/lib
# /usr/local/opt/sqlite/lib/libsqlite3.a
demo: demo.c
	gcc $< \
		-Os \
		-I../../dist/debug \
		-L../../dist/debug \
		-L/usr/local/opt/sqlite/lib \
		-DSQLITE_CORE \
		-Wl,-undefined,dynamic_lookup \
		-lsqlite3 -lsqlite_vector0 -lsqlite_vss0 -lfaiss_avx2 -lomp \
		$(LDFLAGS) \
		-o $@
