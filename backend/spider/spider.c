#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <ctype.h>
#include <sqlite3.h>
#include <curl/curl.h>

#include "../lib/easy_args.h"
#include "../lib/logger.h"

#include "../lib/sqlite_helper.h"

#include "sqlite_conn.h"

#define PROGRAM_NAME "updater"

int QUIT = 0;

struct config {
	int single;
	int verbosity;
	int sleep;
	int hash;
	char* dbpath;
	int tree;
};

void updater_quit() {
	QUIT = 1;
}

int setSleep(int argc, char** argv, void* c) {
	struct config* config = (struct config*) c;
	config->sleep = strtoul(argv[1], NULL, 10);

	return 0;
}

int setDBPath(int argc, char** argv, void* c) {
	struct config* config = (struct config*) c;
	config->dbpath = argv[1];

	return 0;
}

int setSingle(int argc, char** argv, void* c) {
	struct config* config = (struct config*) c;
	config->single = 1;

	return 0;
}

int run(LOGGER log, struct config config) {
	sqlite3* db = sqlite_service_connect(log, config.dbpath);

	curl_global_init(CURL_GLOBAL_SSL);

	if (!db) {
		return 1;
	}

	if (config.single) {
		QUIT = 1;
	}
	
	if (prepare_statements(log, db)) {
		return 2;
	}
	
	spider(log, db, config.hash, config.tree);
	while(!QUIT) {
		spider(log, db, config.hash, config.tree);
		sleep(config.sleep);
	}
	finalize_statements(log);
	sqlite_service_close(log, db);

	curl_global_cleanup();
	return 0;
}

int usage(int argc, char** argv, void* c) {
	printf("usage:\n");
	printf("%s [<options>]\n", PROGRAM_NAME);
	printf("-h, --help\t\tShow this help.\n");
	printf("-s --single\t\tCheck only one time.\n");
	printf("-w --wait\t\tSets the wait time for each cycle.\n");
	printf("-d --dbpath\t\tDatabase path.\n");
	return 1;
}

int setVerbosity(int argc, char** argv, void* c) {
	struct config* config = (struct config*) c;
	config->verbosity = strtoul(argv[1], NULL, 10);

	return 0;
}

int setNoHash(int argc, char** argv, void* c) {
	struct config* config = (struct config*) c;
	config->hash = 0;

	return 0;
}

int setTree(int argc, char** argv, void* c) {

	struct config* config = (struct config*) c;
	config->tree = strtoul(argv[1], NULL, 10);

	return 0;
}

int addArgs() {
	
	eargs_addArgument("-d", "--dbpath", setDBPath, 1);
	eargs_addArgument("-s", "--single", setSingle, 0);
	eargs_addArgument("-h", "--help", usage, 0);
	eargs_addArgument("-n", "--nohash", setNoHash, 0);
	eargs_addArgument("-t", "--tree", setTree, 1);
	eargs_addArgument("-v", "--verbosity", setVerbosity, 1);
	eargs_addArgument("-w", "--wait", setSleep, 1);

	return 0;
}

int main(int argc, char* argv[]) {

	addArgs();

	struct config config = {
		.single = 0,
		.verbosity = 0,
		.sleep = 60,
		.hash = 1,
		.dbpath = "",
		.tree = 0
	};

	char* output[argc];

	eargs_parse(argc, argv, output, &config);

	LOGGER log = {
		.stream = stderr,
		.verbosity = config.verbosity
	};

	signal(SIGQUIT, updater_quit);
	run(log, config);

	return 0;
}
