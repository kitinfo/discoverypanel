#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>

int check_online(const char* host, const char* path) {

	CURL* curl = curl_easy_init();

	char* url = NULL;

	unsigned int host_l = strlen(host);
	unsigned int path_l = strlen(path);

	unsigned int sum_l = host_l + path_l + 1;

	url = malloc(sum_l);

	snprintf(url, sum_l, "%s%s", host, path);


	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
		curl_easy_setopt(curl,CURLOPT_NOBODY ,1 );

		CURLcode res;
		res = curl_easy_perform(curl);

		/* Check for errors */ 
		if (res != CURLE_OK) {
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));
		} else {
			long status; 
			CURLcode code = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE , &status);

			if (code != CURLE_OK) {
				free(url);
				curl_easy_cleanup(curl);
				return -1;
			}
			free(url);
			curl_easy_cleanup(curl);
			return (int) status;
		}
		curl_easy_cleanup(curl);

		free(url);

		return -1;
	}

	free(url);

	return -1;
}
