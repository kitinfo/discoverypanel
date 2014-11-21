#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <ctype.h>
#include "sqlite_conn.h"
#include "parse.h"

int QUIT = 0;
char* tree = NULL;
void updater_quit() {
	QUIT = 1;
}

void setTree(char* id) {
	tree = id;
}

void setSingle() {
	QUIT = 1;
}

void* run() {
	sqlite_service_connect();

	do {
		check(tree);
	} while (!QUIT);
	sqlite_service_close();

	return (void*)0;
}

int main(int argc, char* argv[]) {
	
	setDatabasePath("updater.db3");
	parseArguments(argc, argv);

	pthread_t pid = 0;
	pthread_create(&pid, NULL, run, NULL);

	signal(SIGQUIT, updater_quit);

	pthread_join(pid, NULL);
	free_database_path();
	return 0;
}
