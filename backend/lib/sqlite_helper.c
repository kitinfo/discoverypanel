#include <string.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <stdio.h>
#include "logger.h"

void sqlite_service_close(LOGGER log, sqlite3* db) {

	sqlite3_close(db);
}

sqlite3_stmt* database_prepare(LOGGER log, sqlite3* conn, char* query){
        int status;
        sqlite3_stmt* target=NULL;

        status=sqlite3_prepare_v2(conn, query, strlen(query), &target, NULL);

        switch(status){
                case SQLITE_OK:
                        logprintf(log, LOG_DEBUG, "Statement (%s) compiled ok\n", query);
                        return target;
                default:
                        logprintf(log, LOG_ERROR, "Failed to prepare statement (%s): %s\n", query, sqlite3_errmsg(conn));
        }

        return NULL;
}

void finalize_statement(LOGGER log, sqlite3_stmt* stmt) {
	if (stmt) {
		sqlite3_finalize(stmt);
	}
}

sqlite3* sqlite_service_connect(LOGGER log, char* dbpath) {

	sqlite3* db;
	int rc;

	if (!dbpath || strlen(dbpath) < 1) {
		logprintf(log, LOG_ERROR, "No database path provided\n");
		return NULL;
	}

	rc = sqlite3_open_v2(dbpath, &db, SQLITE_OPEN_READWRITE, NULL);

	if (rc != SQLITE_OK) {
		logprintf(log, LOG_ERROR, "%s\n", sqlite3_errmsg(db));	
		return NULL;
	}

	const char* sql = "PRAGMA foreign_keys = ON";
	
	switch(sqlite3_exec(db, sql, NULL, NULL, NULL)) {
		case SQLITE_OK:
		case SQLITE_DONE:
			break;
		default:
			logprintf(log, LOG_ERROR, "Cannot enable foreign key support (%s).\n", sqlite3_errmsg(db));
			sqlite_service_close(log, db);
			return NULL;
	}

	return db;
}

