demo: demo.c
	gcc $< \
		-I../../dist/debug \
		-L../../dist/debug \
		-DSQLITE_CORE \
		-lvector0 -lvss0 -lfaiss_avx2 \
		-Wl,-undefined,dynamic_lookup \
		-lsqlite3 \
		-o $@
