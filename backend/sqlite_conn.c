#include <string.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <stdio.h>

void debug(char* msg) {
	printf("DEBUG: %s", msg);
}

char* path;
sqlite3* db = NULL;

void setDatabasePath(char* dbpath) {

	path = malloc(sizeof(dbpath));

	strcpy(path, dbpath);
}

void sqlite_service_connect() {

	int rc;
	rc = sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE, NULL);

	if (rc) {
		exit(1);
	}
}

void sqlite_service_close() {

	sqlite3_close(db);
	db = NULL;
}

void update_file(int id, int status_code) {

	if (db == NULL) {
		sqlite_service_connect();
	}

	const char* sql = "UPDATE OR REPLACE files SET status = :status WHERE id = :id ";

	sqlite3_stmt* stmt;

	int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

	if (rc != SQLITE_OK) {
		debug("ERROR on prepared Statement: update");
		return;
	}
	int id_index, status_code_index = 0;

	id_index = sqlite3_bind_parameter_index(stmt, ":id");
	status_code_index = sqlite3_bind_parameter_index(stmt, ":status");

	sqlite3_bind_int(stmt, id_index, id);
	sqlite3_bind_int(stmt, status_code_index, status_code);


	sqlite3_step(stmt);

	sqlite3_finalize(stmt);
}


void check_online_host(int id, const unsigned char* baseurl) {
	//TODO: implement
}

void getAllHosts() {

	if (db == NULL) {
		sqlite_service_connect();
	}

	const char* sql = "SELECT * FROM hosts";

	sqlite3_stmt* stmt;

	int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

	if (rc != SQLITE_OK) {
		debug("ERROR on prepared Satement: get_all_hosts");
	}

	rc = sqlite3_step(stmt);

	if (rc != SQLITE_ROW) {
		return;
	}

	int i = 0;

	int baseurl_i;
	int id_i;
	int count = sqlite3_column_count(stmt);
	for (;i < count; i++) {
		const char* out;
		out = sqlite3_column_table_name(stmt,i);

		if (!strcmp(out, "baseurl")) {
			baseurl_i = i;
		} else if (!strcmp(out, "id")) {
			id_i = i;
		}
	}

	while (rc == SQLITE_ROW) {
		
		const unsigned char* baseurl;
		baseurl = sqlite3_column_text(stmt, baseurl_i);
		int id = sqlite3_column_int(stmt, id_i);
	
		check_online_host(id, baseurl);
		
		rc = sqlite3_step(stmt);
	}

}
