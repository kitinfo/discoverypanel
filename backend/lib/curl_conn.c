#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include "logger.h"


CURL* c_init(LOGGER log, const char* host, const char* path, int get_body) {
	CURL* curl = curl_easy_init();

	if (!curl) {
		logprintf(log, LOG_ERROR, "Cannot init curl.\n");
		return NULL;
	}

	unsigned int host_l = strlen(host);
	unsigned int path_l = strlen(path);

	unsigned int sum_l = host_l + path_l + 1;

	char url[host_l + path_l + 1];

	snprintf(url, sum_l, "%s%s", host, path);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	if (!get_body) {
		curl_easy_setopt(curl, CURLOPT_NOBODY ,1 );
	}

	return curl;
}

int curl_perform(LOGGER log, CURL* curl) {
	CURLcode res = curl_easy_perform(curl);

	/* Check for errors */ 
	if (res != CURLE_OK) {
		logprintf(log, LOG_ERROR, "curl_easy_perform() failed: %s\n",
		curl_easy_strerror(res));
		return 0;
	}

	return 1;
}

int get_request(LOGGER log, const char* host, const char* path) {

	CURL* curl = c_init(log, host, path, 1);

	if (!curl) {
		return 0;
	}

	curl_easy_setopt(curl, CURLOPT_DIRLISTONLY, 1);

	if (!curl_perform(log, curl)) {
		curl_easy_cleanup(curl);
		return 0;
	}

	return 1;
}

int check_online(LOGGER log, const char* host, const char* path) {

	CURL* curl = c_init(log, host, path, 0);

	if (!curl) {
		return -1;
	}

	if (!curl_perform(log, curl)) {
		curl_easy_cleanup(curl);
		return -1;
	}

	long status; 
	CURLcode code = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE , &status);

	if (code != CURLE_OK) {
		curl_easy_cleanup(curl);
		return -1;
	}
	curl_easy_cleanup(curl);
	return (int) status;

}
