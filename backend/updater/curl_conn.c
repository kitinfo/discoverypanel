#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include "logger.h"

int check_online(LOGGER log, const char* host, const char* path) {

	CURL* curl = curl_easy_init();


	unsigned int host_l = strlen(host);
	unsigned int path_l = strlen(path);

	unsigned int sum_l = host_l + path_l + 1;

	char url[host_l + path_l + 1];

	snprintf(url, sum_l, "%s%s", host, path);

	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
		curl_easy_setopt(curl, CURLOPT_NOBODY ,1 );

		CURLcode res;
		res = curl_easy_perform(curl);

		/* Check for errors */ 
		if (res != CURLE_OK) {
			logprintf(log, LOG_ERROR, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));
		} else {
			long status; 
			CURLcode code = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE , &status);

			if (code != CURLE_OK) {
				curl_easy_cleanup(curl);
				return -1;
			}
			curl_easy_cleanup(curl);
			return (int) status;
		}
		curl_easy_cleanup(curl);

		return -1;
	}

	return -1;
}
