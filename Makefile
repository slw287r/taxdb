taxdb: taxdb.c
	cc -o $@ $<

clean:
	rm -f taxdb
