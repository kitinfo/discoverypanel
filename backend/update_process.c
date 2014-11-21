#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include "sqlite_conn.h"

int QUIT = 0;

void updater_quit() {
	QUIT = 1;
}

void run() {
	sqlite_service_connect();

	while (!QUIT) {

		getAllHosts();

	}
	sqlite_service_close();
}

int main(argc, argv) {
	
//	parseArguments(argc, argv);

	setDatabasePath("update.db3");

	run();

//	pthread_t pid = 0;

//	pthread_create(&pid, NULL, run, NULL);


//	signal(SIGHUP, force_refresh);
	signal(SIGQUIT, updater_quit);

//	pthread_join(pid, NULL);

	return 0;
}
