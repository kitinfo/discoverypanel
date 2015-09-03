#pragma once
#include <sqlite3.h>
#include "../lib/logger.h"

int prepare_statements(LOGGER log, sqlite3* db);
void finalize_statements(LOGGER log);
void check(LOGGER log, sqlite3* db, char* tree_id, int quick);
