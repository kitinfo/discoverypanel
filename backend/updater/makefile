.PHONY: all updater clean

CARGS=-Wall -g
LDLIBS=-lsqlite3 -lcurl

all: updater

updater: 
	$(CC) $(CARGS) $(LDLIBS) update_process.c sqlite_conn.c curl_conn.c easy_args.c logger.c
	mv a.out updater

clean:
	$(RM) update_process

