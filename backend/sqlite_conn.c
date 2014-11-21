#include <string.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <stdio.h>
#include "curl_conn.h"

void debug(char* msg) {
	printf("DEBUG: %s\n", msg);
}

char* path = NULL;
sqlite3* db = NULL;

void free_database_path() {
	if (path != NULL) {
		free(path);
	}
}

void setDatabasePath(char* dbpath) {

	free_database_path();
	path = malloc(strlen(dbpath) + 1);

	strcpy(path, dbpath);
	debug(path);
}

void sqlite_service_connect() {

	int rc;
	rc = sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE, NULL);

	if (rc != SQLITE_OK) {
		printf("ERROR: %s\n", sqlite3_errmsg(db));	
		exit(1);
	}

	sqlite3_stmt* stmt;

	const char* sql = "PRAGMA foreign_keys = ON";
	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

	if (rc != SQLITE_OK) {
		return;
	}

	sqlite3_step(stmt);
}

void sqlite_service_close() {

	sqlite3_close(db);
	db = NULL;
}


void update(char* id, const char* sql, int status_code) {

	if (db == NULL) {
		sqlite_service_connect();
	}

	sqlite3_stmt* stmt;

	int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

	if (rc != SQLITE_OK) {
		debug("ERROR on prepared Statement: update");
		return;
	}
	int id_index, status_code_index = 0;

	id_index = sqlite3_bind_parameter_index(stmt, ":id");
	status_code_index = sqlite3_bind_parameter_index(stmt, ":status");

	sqlite3_bind_text(stmt, id_index, id, -1, NULL);
	sqlite3_bind_int(stmt, status_code_index, status_code);


	sqlite3_step(stmt);

	sqlite3_finalize(stmt);
}

void update_host(char* id, int status_code) {

	const char* sql = "UPDATE OR REPLACE hosts SET status = :status WHERE id = :id";
	update(id, sql, status_code);
}

void update_file(int id, int status_code) {

	const char* sql = "UPDATE OR REPLACE files SET status = :status WHERE id = :id ";
	
	char sid[sizeof(id)];
	sprintf(sid, "%d", id);
	update(sid, sql, status_code);
}

void check_online_file(int id, const char* baseurl, const char* path) {
	printf("DEBUG: %s\n",baseurl);
	printf("DEBUG: %s\n",path);
	
	int status = check_online(baseurl, path);

	printf("HTTP-STATUS file (%s%s): %d\n", baseurl, path, status);

	update_file(id, status);
}

void check_online_files(int tree_id, const char* baseurl) {

	if (db == NULL) {
		sqlite_service_connect();
	}

	const char* sql = "SELECT file_id, path FROM files WHERE tree_id = :tree";

	sqlite3_stmt* stmt;

	int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

	if (rc != SQLITE_OK) {
		return;
	}

	int tree_index = 0;
	tree_index = sqlite3_bind_parameter_index(stmt, ":tree");
	sqlite3_bind_int(stmt, tree_index, tree_id);

	rc = sqlite3_step(stmt);

	if (rc == SQLITE_DONE) {
		debug("No rows found in files for given id");
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

		check_online_file(file_id, baseurl, path);

		rc = sqlite3_step(stmt);
	}

	sqlite3_finalize(stmt);
}

void check_online_host(int id, const char* baseurl) {

	const char* file = "";

	int status = check_online(baseurl, file);

	printf("HTTP-STATUSi host: %d\n",status);

	char sid[sizeof(id)];
	sprintf(sid, "%d", id);

	update_host(sid, status);

	if (status == 200 || status == 212) {
		check_online_files(id, baseurl);
	}

}

void check(char* tree_id) {

	if (db == NULL) {
		sqlite_service_connect();
	}
	const char* sql;
	if (tree_id == NULL) {

		sql = "SELECT id, base FROM trees";
	} else {
		sql = "SELECT id, base FROM trees WHERE id = :id"; 
	}
	sqlite3_stmt* stmt;

	int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

	if (rc != SQLITE_OK) {
		debug("ERROR on prepared Satement: get_all_hosts");
	}

	if (tree_id != NULL) {
		int tree_index = sqlite3_bind_parameter_index(stmt, ":id");
		sqlite3_bind_text(stmt, tree_index, tree_id, -1, NULL);
	}


	rc = sqlite3_step(stmt);
	
	if (rc == SQLITE_DONE) {
		debug("WARNING: no rows found in hosts");
		return;
	}


	if (rc != SQLITE_ROW) {
		return;
	}

	int i = 0;

	int baseurl_i = -1;
	int id_i = -1;
	int count = sqlite3_column_count(stmt);
	for (;i < count; i++) {
		const char* out;
		out = sqlite3_column_name(stmt,i);
		printf("DEBUG: out = %s\n", out);
		if (!strcmp(out, "base")) {
			baseurl_i = i;
		} else if (!strcmp(out, "id")) {
			id_i = i;
		}
	}

	while (rc == SQLITE_ROW) {
		
		const char* baseurl = (const char*) sqlite3_column_text(stmt, baseurl_i);
		printf("DEBUG: %s\n", baseurl);
		int id = sqlite3_column_int(stmt, id_i);
		check_online_host(id, baseurl);
		
		rc = sqlite3_step(stmt);
	}

	sqlite3_finalize(stmt);
}
