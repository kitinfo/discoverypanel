.PHONY: all updater clean

CARGS=-Wall -g
LDLIBS=-lsqlite3 -lcurl

all: updater

updater: 
	$(CC) $(CARGS) $(LDLIBS) update_process.c sqlite_conn.c ../lib/sqlite_helper.c ../lib/curl_conn.c ../lib/easy_args.c ../lib/logger.c
	mv a.out updater

clean:
	$(RM) update_process

