int prepare_statements(LOGGER log, sqlite3* db);
int finalize_statements(LOGGER log);
int spider(LOGGER log, sqlite3* db, int hash, int tree);
