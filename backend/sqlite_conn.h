#pragma once

void free_database_path();
void sqlite_service_connect();
void sqlite_service_close();
void check(char* tree_id, int quick);
void setDatabasePath(char* dbpath);
