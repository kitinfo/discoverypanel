#include <string.h>
#include <sqlite3.h>
#include "../lib/logger.h"

#include "../lib/curl_conn.h"
#include "../lib/sqlite_helper.h"

sqlite3_stmt* stmt_insert_file;
sqlite3_stmt* stmt_select_trees;
sqlite3_stmt* stmt_update_hash;
sqlite3_stmt* stmt_select_tree;

struct request_config {
	LOGGER log;
	int tree;
	int hash;
	const char* base;
	char* path;
	sqlite3* db;
};

int prepare_statements(LOGGER log, sqlite3* db) {
	if (!db) {
		return 1;
	}

	stmt_update_hash = database_prepare(log, db, "UPDATE OR IGNORE files SET hash = ? WHERE tree_id = ? AND path = ?");
	stmt_insert_file = database_prepare(log, db, "INSERT OR IGNORE INTO files (tree_id, path, status) VALUES(?, ?, ?)");
	stmt_select_trees = database_prepare(log, db, "SELECT id, base, spidered FROM trees");
	stmt_select_tree = database_prepare(log, db, "SELECT base FROM trees WHERE id = ?");
	
	return 0;
}

int finalize_statements(LOGGER log) {
	finalize_statement(log, stmt_select_trees);
	finalize_statement(log, stmt_select_tree);
	finalize_statement(log, stmt_insert_file);
	finalize_statement(log, stmt_update_hash);

	return 0;
}

int find_token(char* s, char delim) {
	int pos;
	for (pos = 0; s[pos] != '\0'; pos++) {
		if (s[pos] == delim) {
			return pos;
		}
	}

	return -1;
}

int hash_file(struct request_config* rc) {

	return 0;
}

int handle_file(struct request_config* rc, char* path) {

	sqlite3_bind_int(stmt_insert_file, 1, rc->tree);
	sqlite3_bind_text(stmt_insert_file, 2, path, -1, NULL);
	sqlite3_bind_int(stmt_insert_file, 3, 200);

	int ret = sqlite3_step(stmt_insert_file);

	if (ret != SQLITE_DONE) {
		logprintf(rc->log, LOG_ERROR, "Error in inserting file: %s\n", sqlite3_errmsg(rc->db));
	} else {
		logprintf(rc->log, LOG_INFO, "File (%s) added successfully to database.\n", path);

		if (rc->hash) {
			hash_file(rc);
		}
	}

	sqlite3_reset(stmt_insert_file);
	sqlite3_clear_bindings(stmt_insert_file);

	return 0;
}

inline size_t handle_request_data(void* data, size_t size, size_t nmemb, void* userp);
int handle_dir(struct request_config* rc, char* path) {

	struct request_config rc_new = {
		.log = rc->log,
		.tree = rc->tree,
		.hash = rc->hash,
		.path = path,
		.base = rc->base,
		.db = rc->db
	};

	int status = get_request(rc->log, rc->base, path, handle_request_data, &rc_new);
	return status;
}

int ignore_link(char* link) {


	// parent directory
	if (!strcmp(link, "../")) {
		return 1;
	}

	// same directory
	if (!strcmp(link, "./")) {
		return 1;
	}

	// git directory
	if (!strcmp(link, ".git/")) {
		return 1;
	}

	// root directory (often parent)
	if (!strcmp(link, "/")) {
		return 1;
	}

	// svn directory
	if (!strcmp(link, ".svn/")) {
		return 1;
	}

	// external link
	if (strstr(link, "://")) {
		return 1;
	}

	return 0;
}


size_t handle_request_data(void* data, size_t size, size_t nmemb, void* userp) {

	struct request_config* rc = (struct request_config*) userp;

	logprintf(rc->log, LOG_DEBUG, "output: %s\n", (char*) data);

	char* current = (char*) data;
	int end;
	while((current = strstr(current, "href="))) {
		current = current + 6;

		end = find_token(current, '\"');
		if (end > 0) {
			char link[end + 1];
			memset(link, 0, end + 1);
			strncpy(link, current, end);
			logprintf(rc->log, LOG_INFO, "Link found: %s%s\n", rc->path,link);

			if (!ignore_link(link)) {

				char linkpath[strlen(rc->path) + strlen(link) + 1];
				memset(linkpath, 0, sizeof(linkpath));
				snprintf(linkpath, sizeof(linkpath), "%s%s",rc->path, link);

				logprintf(rc->log, LOG_DEBUG, "Not the parent link.\n");
				logprintf(rc->log, LOG_DEBUG, "Last character: %c\n", link[end - 1]);
				if (link[end - 1] == '/') {
					logprintf(rc->log, LOG_DEBUG, "Link is a directory.\n");
					handle_dir(rc, linkpath);
				} else {
					logprintf(rc->log, LOG_DEBUG, "Link is a file.\n");
					handle_file(rc, linkpath);
				}
			} else {
				logprintf(rc->log, LOG_DEBUG, "Found the parent link, ignore.\n");
			}
		}
	}


	return size * nmemb;
}

int spider_files(LOGGER log, sqlite3* db, int hash, int tree, const char* base) {

	struct request_config rc = {
		.log = log,
		.tree = tree,
		.hash = hash,
		.path = "",
		.base = base,
		.db = db
	};

	int status = get_request(log, base, "", handle_request_data, &rc);

	return status;
}

int spider_tree(LOGGER log, sqlite3* db, int hash, int tree) {

	sqlite3_bind_int(stmt_select_tree, 1, tree);

	int status = sqlite3_step(stmt_select_tree);	

	if (status == SQLITE_DONE) {
		logprintf(log, LOG_WARNING, "Tree with id %d not found.\n", tree);
		return -1;
	}

	const char* base = (const char*) sqlite3_column_text(stmt_select_tree, 0);

	logprintf(log, LOG_DEBUG, "Base found: %s\n", base);

	status = spider_files(log, db, hash, tree, base);

	sqlite3_reset(stmt_select_tree);
	sqlite3_clear_bindings(stmt_select_tree);

	return status;
}

int spider(LOGGER log, sqlite3* db, int hash, int tree) {
	
	if (!db) {
		return 1;
	}

	if (tree > 0) {
		return spider_tree(log, db, hash, tree);
	}

	int status = sqlite3_step(stmt_select_trees);

	if (status == SQLITE_DONE) {
		logprintf(log, LOG_WARNING, "No tree found.\n", tree);
		return -1;
	}

	const char* base;
	int spidered;

	while (status != SQLITE_DONE) {

		tree = sqlite3_column_int(stmt_select_trees, 0);
		base = (const char*) sqlite3_column_text(stmt_select_trees, 1);
		spidered = sqlite3_column_int(stmt_select_trees, 2);

		logprintf(log, LOG_INFO, "Base found: %s\n", base);
		
		if (!spidered) {
			if (spider_files(log, db, hash, tree, base) < 0) {
				return 2;
			}
		}

		status = sqlite3_step(stmt_select_trees);
	}

	sqlite3_reset(stmt_select_trees);
	sqlite3_clear_bindings(stmt_select_trees);

	return 0;
}
