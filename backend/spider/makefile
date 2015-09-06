.PHONY: all spider clean

CARGS=-Wall -g
LDLIBS=-lsqlite3 -lcurl

all: spider

spider: 
	$(CC) $(CARGS) $(LDLIBS) spider.c sqlite_conn.c ../lib/sqlite_helper.c ../lib/curl_conn.c ../lib/easy_args.c ../lib/logger.c
	mv a.out spider

clean:
	$(RM) spider

