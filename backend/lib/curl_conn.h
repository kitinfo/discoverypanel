#pragma once
#include "logger.h"

int check_online(LOGGER log, const char* host, const char* path);
int get_request(LOGGER log, const char* host, const char* path);
