#include <sqlite3.h>
#include "../lib/logger.h"

#include "../lib/sqlite_helper.h"

sqlite3_stmt* stmt_insert_file;
sqlite3_stmt* stmt_select_trees;
sqlite3_stmt* stmt_insert_file_wo_hash;
sqlite3_stmt* stmt_select_tree;

int prepare_statements(LOGGER log, sqlite3* db) {
	if (!db) {
		return 1;
	}

	stmt_insert_file = database_prepare(log, db, "INSERT INTO file (tree_id, path, status, hash) VALUES(?, ?, ?, ?)");
	stmt_insert_file_wo_hash = database_prepare(log, db, "INSERT INTO file (tree_id, path, status) VALUES(?, ?, ?)");
	stmt_select_trees = database_prepare(log, db, "SELECT id, base, spidered FROM trees");
	stmt_select_tree = database_prepare(log, db, "SELECT base FROM trees WHERE id = ?");
	
	return 0;
}

int finalize_statements(LOGGER log) {
	finalize_statement(log, stmt_select_trees);
	finalize_statement(log, stmt_select_tree);
	finalize_statement(log, stmt_insert_file);
	finalize_statement(log, stmt_insert_file_wo_hash);

	return 0;
}

int spider_file(LOGGER log, sqlite3* db, int hash, int tree, const char* base) {

	return 0;
}

int spider_files(LOGGER log, sqlite3* db, int hash, int tree, const char* base) {



	return 0;
}

int spider_tree(LOGGER log, sqlite3* db, int hash, int tree) {

	sqlite3_bind_int(stmt_select_tree, 1, tree);

	int status = sqlite3_step(stmt_select_tree);	

	if (status == SQLITE_DONE) {
		logprintf(log, LOG_WARNING, "Tree with id %d not found.\n", tree);
		return -1;
	}

	const char* base = (const char*) sqlite3_column_text(stmt_select_tree, 1);

	status = spider_files(log, db, hash, tree, base);

	sqlite3_reset(stmt_select_tree);
	sqlite3_clear_bindings(stmt_select_tree);

	return status;
}

int spider(LOGGER log, sqlite3* db, int hash, int tree) {
	
	if (tree > 0) {
		return spider_tree(log, db, hash, tree);
	}

	if (!db) {
		return 1;
	}

	return 0;
}
