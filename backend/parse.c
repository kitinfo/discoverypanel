#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "sqlite_conn.h"
#include "update_process.h"

void print_help() {
	printf("usage: ");
}

void parseArguments(int argc, char* argv[]) {

    //parse arguments
    int i;
    for (i = 1; i < argc; i++) {
        if (*argv[i] == '-') {
            if (i + 1 < argc && (!strcmp(argv[i], "--tree") || !strcmp(argv[i], "-t"))) {
                setTree(argv[i + 1]);
                i++;
            }  else if (!strcmp(argv[i], "--single") || !strcmp(argv[i], "-s")) {
                setSingle();
            }  else if (!strcmp(argv[i], "--quick") || !strcmp(argv[i], "-q")) {
                setQuick();
            }  else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
                print_help();
            } else if (i + 1 < argc && (!strcmp(argv[i], "--database") || !strcmp(argv[i], "-b"))) {
                setDatabasePath(argv[i + 1]);
		i++;
            } else {
                printf("Invalid arguments.\n");
                print_help();
            }
        }
    }
}
