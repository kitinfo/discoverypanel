#pragma once
#include <sqlite3.h>
#include "logger.h"

sqlite3* sqlite_service_connect(LOGGER log, char* dbpath);
int prepare_statements(LOGGER log, sqlite3* db);
void finalize_statements(LOGGER log);
void sqlite_service_close(LOGGER log,sqlite3* db);
void check(LOGGER log, sqlite3* db, char* tree_id, int quick);
