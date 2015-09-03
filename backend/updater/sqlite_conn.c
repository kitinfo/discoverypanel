#include <string.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <stdio.h>
#include "../lib/curl_conn.h"
#include "../lib/logger.h"
#include "../lib/sqlite_helper.h"

sqlite3_stmt* stmt_select_trees = NULL;
sqlite3_stmt* stmt_select_tree = NULL;
sqlite3_stmt* stmt_update_tree_node = NULL;
sqlite3_stmt* stmt_update_file = NULL;
sqlite3_stmt* stmt_select_files = NULL;

int prepare_statements(LOGGER log, sqlite3* db) {
	
	stmt_select_files = database_prepare(log, db, "SELECT file_id, path FROM files WHERE tree_id = :tree");
	stmt_select_trees = database_prepare(log, db, "SELECT id, base FROM trees");
	stmt_select_tree = database_prepare(log, db, "SELECT id, base FROM trees WHERE id = :id");
	stmt_update_file = database_prepare(log, db, "UPDATE OR REPLACE files SET status = :status WHERE file_id = :id");
	stmt_update_tree_node = database_prepare(log, db, "UPDATE OR REPLACE trees SET status = :status WHERE id = :id");

	return 0;
}

int finalize_statements(LOGGER log) {
	finalize_statement(log, stmt_select_trees);
	finalize_statement(log, stmt_select_tree);
	finalize_statement(log, stmt_update_tree_node);
	finalize_statement(log, stmt_update_file);
	finalize_statement(log, stmt_select_files);

	return 0;
}

void update(LOGGER log, sqlite3* db, char* id, sqlite3_stmt* stmt, int status_code) {

	if (!db) {
		return;
	}

	int id_index, status_code_index = 0;

	id_index = sqlite3_bind_parameter_index(stmt, ":id");
	status_code_index = sqlite3_bind_parameter_index(stmt, ":status");

	sqlite3_bind_text(stmt, id_index, id, -1, NULL);
	sqlite3_bind_int(stmt, status_code_index, status_code);


	sqlite3_step(stmt);
	sqlite3_reset(stmt);
	sqlite3_clear_bindings(stmt);
}

void update_host(LOGGER log, sqlite3* db, char* id, int status_code) {

	update(log, db, id, stmt_update_tree_node, status_code);
}

void update_file(LOGGER log, sqlite3* db, int id, int status_code) {

	char sid[sizeof(id)];
	sprintf(sid, "%d", id);
	update(log, db, sid, stmt_update_file, status_code);
}

void check_online_file(LOGGER log, sqlite3* db, int id, const char* baseurl, const char* path) {
	logprintf(log, LOG_INFO, "baseurl: %s\n",baseurl);
	logprintf(log, LOG_INFO, "path: %s\n",path);
	
	int status = check_online(log, baseurl, path);

	logprintf(log, LOG_DEBUG, "HTTP-STATUS file (%s%s): %d\n", baseurl, path, status);

	update_file(log, db, id, status);
}

void check_online_files(LOGGER log, sqlite3* db, int tree_id, const char* baseurl) {

	if (!db) {
		return;
	}
	int rc;
	sqlite3_stmt* stmt = stmt_select_files;

	int tree_index = 0;
	tree_index = sqlite3_bind_parameter_index(stmt, ":tree");
	sqlite3_bind_int(stmt, tree_index, tree_id);

	rc = sqlite3_step(stmt);

	if (rc == SQLITE_DONE) {
		logprintf(log, LOG_WARNING, "No rows found in files for given id\n");
		return;
	}

	int i = 0;
	int count = sqlite3_column_count(stmt);

	int id_i = 0;
	int path_i = 0;

	for (; i < count; i++) {
		const char* out = sqlite3_column_name(stmt, i);

		if (!strcmp(out, "tree_id")) {
			id_i = i;
		} else if (!strcmp(out, "path")) {
			path_i = i;
		}
	}

	while (rc == SQLITE_ROW) {
		const char* path = (const char*) sqlite3_column_text(stmt, path_i);
		int file_id = sqlite3_column_int(stmt, id_i);

		check_online_file(log, db, file_id, baseurl, path);

		rc = sqlite3_step(stmt);
	}
	
	sqlite3_reset(stmt);
	sqlite3_clear_bindings(stmt);
}

void check_online_host(LOGGER log, sqlite3* db, int id, const char* baseurl, int quick) {

	const char* file = "";

	int status = check_online(log, baseurl, file);

	logprintf(log, LOG_DEBUG, "HTTP-STATUS host: %d\n",status);

	char sid[sizeof(id)];
	sprintf(sid, "%d", id);

	update_host(log, db, sid, status);

	if (status == 200 || status == 212) {

		if (!quick) {
			check_online_files(log, db, id, baseurl);
		}
	}

}

int check(LOGGER log, sqlite3* db, char* tree_id, int quick) {

	// check database connection
	if (!db) {
		logprintf(log, LOG_ERROR, "No database connection provided.\n");
		return -1;
	}
	int rc;
	
	sqlite3_stmt* stmt = stmt_select_trees;

	if (!tree_id) {	
		stmt = stmt_select_tree;
		int tree_index = sqlite3_bind_parameter_index(stmt, ":id");
		sqlite3_bind_text(stmt, tree_index, tree_id, -1, NULL);
	}
	
	rc = sqlite3_step(stmt);
	
	
	// no hosts
	if (rc == SQLITE_DONE) {
		logprintf(log, LOG_WARNING, "No hosts in database\n");
		return 0;
	}

	if (rc != SQLITE_ROW) {
		logprintf(log, LOG_ERROR, "Error in getting hosts from database (%s)\n", sqlite3_errmsg(db));
		return -1;
	}

	int i = 0;

	int baseurl_i = -1;
	int id_i = -1;
	int count = sqlite3_column_count(stmt);
	for (;i < count; i++) {
		const char* out;
		out = sqlite3_column_name(stmt,i);
		logprintf(log, LOG_INFO, "out = %s\n", out);
		if (!strcmp(out, "base")) {
			baseurl_i = i;
		} else if (!strcmp(out, "id")) {
			id_i = i;
		}
	}

	while (rc == SQLITE_ROW) {
		
		const char* baseurl = (const char*) sqlite3_column_text(stmt, baseurl_i);
		logprintf(log, LOG_INFO, "baseurl: %s\n", baseurl);
		int id = sqlite3_column_int(stmt, id_i);
		check_online_host(log, db, id, baseurl, quick);
		
		rc = sqlite3_step(stmt);
	}

	sqlite3_reset(stmt);
	sqlite3_clear_bindings(stmt);

	return 0;
}
