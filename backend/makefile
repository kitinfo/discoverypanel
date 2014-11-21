.PHONY: all

all:
	$(CC) -Wall -l sqlite3 -l curl -l pthread -o updater update_process.c sqlite_conn.c curl_conn.c parse.c

debug:
	$(CC) -Wall -l sqlite3 -l curl -l pthread -o updater -g update_process.c sqlite_conn.c curl_conn.c parse.c
