#pragma once
#include <sqlite3.h>
#include "logger.h"

sqlite3* sqlite_service_connect(LOGGER log, char* dbpath);
sqlite3_stmt* database_prepare(LOGGER log, sqlite3* db, const char* sql);
void finalize_statement(LOGGER log, sqlite3_stmt* stmt);
void sqlite_service_close(LOGGER log,sqlite3* db);
void begin_transaction(LOGGER log, sqlite3* db);
void commit_transaction(LOGGER log, sqlite3* db);
