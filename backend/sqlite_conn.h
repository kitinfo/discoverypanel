#pragma once

void sqlite_service_connect();
void sqlite_service_close();
void sqlite_service_update(int id, int status_code);
void check_online_host(int id, char* baseurl);
void getAllHosts();
void setDatabasePath(char* dbpath);
