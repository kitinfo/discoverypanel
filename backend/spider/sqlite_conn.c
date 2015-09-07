#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "../lib/logger.h"

#include "../lib/curl_conn.h"
#include "../lib/sqlite_helper.h"

sqlite3_stmt* stmt_insert_file;
sqlite3_stmt* stmt_select_trees;
sqlite3_stmt* stmt_update_hash;
sqlite3_stmt* stmt_select_tree;
sqlite3_stmt* stmt_insert_tag;
sqlite3_stmt* stmt_insert_tagmap;
sqlite3_stmt* stmt_select_file_id;
struct tag {
	char* tag;
	struct tag* next;
	struct tag* previous;
};

struct request_config {
	LOGGER log;
	int tree;
	int hash;
	const char* base;
	char* path;
	struct tag* tags;
	sqlite3* db;
};

int prepare_statements(LOGGER log, sqlite3* db) {
	if (!db) {
		return 1;
	}

	stmt_select_file_id = database_prepare(log, db, "SELECT file_id FROM files WHERE path = ? AND tree_id = ?");
	stmt_insert_tag = database_prepare(log, db, "INSERT OR IGNORE INTO tags (tag_text) VALUES(?)");
	stmt_insert_tagmap = database_prepare(log, db, "INSERT OR IGNORE INTO tagmap (file, tag) VALUES (?, (SELECT tag_id FROM tags WHERE tag_text = ?))");
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
	finalize_statement(log, stmt_insert_tag);
	finalize_statement(log, stmt_insert_tagmap);
	finalize_statement(log, stmt_select_file_id);

	return 0;
}

struct tag* copy_tags(struct tag* tags) {

	struct tag* output = NULL;
	struct tag* curr = tags;
	struct tag* o = NULL;

	while (curr) {

		struct tag* o2 = (struct tag*) malloc(sizeof(struct tag));
		o2->tag = malloc(strlen(curr->tag) + 1);
		strcpy(o2->tag, curr->tag);
		
		if (!o) {
			output = o2;
			o2->previous = NULL;
		} else {
			o2->previous = o;
		}
		o2->next = NULL;
		o = o2;

		curr = curr->next;
	}

	return output;
}

int get_file_id(LOGGER log, int tree, char* path) {
	
	sqlite3_bind_text(stmt_select_file_id, 1, path, -1, NULL);
	sqlite3_bind_int(stmt_select_file_id, 2, tree);

	int ret = sqlite3_step(stmt_select_file_id);

	if (ret == SQLITE_DONE) {
		logprintf(log, LOG_INFO, "File with tree=%d and path=(%s) not found\n", tree, path);
		return 0;
	}
	int id = sqlite3_column_int(stmt_select_file_id, 0);

	sqlite3_reset(stmt_select_file_id);
	sqlite3_clear_bindings(stmt_select_file_id);

	logprintf(log, LOG_DEBUG, "File found with id: %d\n", id);

	return id;

}

int insert_tagmap(LOGGER log, sqlite3* db, int file, struct tag* tag) {

	if (!tag) {
		logprintf(log, LOG_INFO, "No tags found!\n");
		return 0;
	}

	while (tag) {
		logprintf(log, LOG_DEBUG, "Add tagmap: (%d, %s)\n", file, tag->tag);
		sqlite3_bind_int(stmt_insert_tagmap, 1, file);
		sqlite3_bind_text(stmt_insert_tagmap, 2, tag->tag, -1, NULL);

		if (sqlite3_step(stmt_insert_tagmap) != SQLITE_DONE) {
			logprintf(log, LOG_INFO, "Error on inserting tagmap: %s", sqlite3_errmsg(db));
		}
		
		sqlite3_reset(stmt_insert_tagmap);
		sqlite3_clear_bindings(stmt_insert_tagmap);

		tag = tag->next;
	}

	return 0;
}

int insert_tag(LOGGER log, sqlite3* db, char* tag) {
	logprintf(log, LOG_INFO, "Insert tag: %s\n", tag);

	sqlite3_bind_text(stmt_insert_tag, 1, tag, -1, NULL);

	sqlite3_step(stmt_insert_tag);

	sqlite3_reset(stmt_insert_tag);
	sqlite3_clear_bindings(stmt_insert_tag);

	return 0;
}

int free_tags(struct request_config* rc) {

	struct tag* curr = rc->tags;
	struct tag* next;

	while (curr) {
		next = curr->next;
		free(curr->tag);
		free(curr);
		curr = next;
	}

	return 0;
}

int remove_tag(struct request_config* rc, char* tag) {
	struct tag* last = rc->tags;
	
	while (last) {

		// check for test
		if (!strcmp(last->tag, tag)) {
			free(last->tag);
			
			if (last->previous && last->next) {
				last->next->previous = last->previous;
				last->previous->next = last->next;
			} else if (last->previous && !last->next) {
				last->previous->next = NULL;
			} else if (!last->previous && last->next) {
				rc->tags = last->next;
				last->next->previous = NULL;
			} else {
				rc->tags = NULL;
			}

			free(last);
			return 0;
		}

		last = last->next;
	}

	return 0;
}

int add_tag(struct request_config* rc, char* tag) {
	struct tag* last = rc->tags;

	if (last) {

		if (!strcmp(last->tag, tag)) {
			return 0;
		}

		while (last->next) {
			if (!strcmp(last->tag, tag)) {
				return 0;
			}

			last = last->next;
		}
	
		last->next = (struct tag*) malloc(sizeof(struct tag));
		last->next->previous = last;
		last->next->next = NULL;
		last->next->tag = (char*) malloc(strlen(tag) + 1);
		strcpy(last->next->tag, tag);
		insert_tag(rc->log, rc->db, tag);
	} else {
		last = (struct tag*) malloc(sizeof(struct tag));
		last->next = NULL;
		last->previous = NULL;
		last->tag = (char*) malloc(strlen(tag) +1);
		strcpy(last->tag, tag);
		rc->tags = last;
		insert_tag(rc->log, rc->db, tag);
	}

	
	return 0;
}

size_t handle_tags(void* data, size_t size, size_t nmemb, void* userp) {

	struct request_config* rc = (struct request_config*) userp;
	char* tags = (char*) data;

	char* curr = strtok(tags, "\n");

	if (!curr) {
		logprintf(rc->log, LOG_INFO, "No tag found or format not valid.\n");
		return size * nmemb;
	}

	do {
	
		logprintf(rc->log, LOG_INFO, "Found tag: %s\n", curr);
		switch(curr[0]) {
			case '-':
				logprintf(rc->log, LOG_DEBUG, "Remove tag: %s\n", curr + 1);
				remove_tag(rc, curr + 1);
				break;
			case '+':
				logprintf(rc->log, LOG_DEBUG, "Add tag: %s\n", curr + 1);
				add_tag(rc, curr + 1);
				break;
			default:
				logprintf(rc->log, LOG_DEBUG, "Add tag: %s\n", curr);
				add_tag(rc, curr);
				break;
		}
	} while ((curr = strtok(NULL, "\n"))); 

	return size * nmemb;
}

int get_tags(struct request_config* rc) {

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
	// on zero it will be ignored in handle_request_data
	int tok = find_token(path, '#');
	if ( tok > 0) {
		path[tok] = '\0';
	}

	sqlite3_bind_int(stmt_insert_file, 1, rc->tree);
	sqlite3_bind_text(stmt_insert_file, 2, path, -1, NULL);
	sqlite3_bind_int(stmt_insert_file, 3, 200);

	int ret = sqlite3_step(stmt_insert_file);

	if (ret != SQLITE_DONE) {
		logprintf(rc->log, LOG_ERROR, "Error in inserting file: %s\n", sqlite3_errmsg(rc->db));
	} else {
		logprintf(rc->log, LOG_INFO, "File (%s) added successfully to database.\n", path);

		int file_id = get_file_id(rc->log, rc->tree, path);
		insert_tagmap(rc->log, rc->db, file_id, rc->tags);

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
		.tags = copy_tags(rc->tags),
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
	if (link[0] == '/') {
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

	// anker link
	if (link[0] == '#') {
		return 1;
	}

	// tags file
	if (!strcmp(link, ".dp_tags")) {
		return 1;
	}

	return 0;
}


size_t handle_request_data(void* data, size_t size, size_t nmemb, void* userp) {


	struct request_config* rc = (struct request_config*) userp;

	logprintf(rc->log, LOG_DEBUG, "output: %s\n", (char*) data);


	char tags_path[strlen(rc->path) + strlen(".dp_tags") + 1];
	memset(tags_path, 0, sizeof(tags_path));

	snprintf(tags_path, sizeof(tags_path), "%s%s", rc->path, ".dp_tags");
	// fill tags
	get_request(rc->log, rc->base, tags_path, handle_tags, userp); 

	int i = 0;

	struct tag* tag = rc->tags;
	while (tag != NULL) {
		i++;
		tag = tag->next;
	}

	logprintf(rc->log, LOG_DEBUG, "Current tag size: %d\n", i);

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

	free_tags(rc);

	return size * nmemb;
}

int spider_files(LOGGER log, sqlite3* db, int hash, int tree, const char* base) {

	struct request_config rc = {
		.log = log,
		.tree = tree,
		.hash = hash,
		.path = "",
		.tags = NULL,
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
