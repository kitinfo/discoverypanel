.PHONY: all spider clean

CARGS=-Wall -g
LDLIBS=-lsqlite3 -lcurl

all: spider

updater: 
	$(CC) $(CARGS) $(LDLIBS) spider_process.c sqlite_conn.c curl_conn.c easy_args.c logger.c
	mv a.out spider

clean:
	$(RM) spider

