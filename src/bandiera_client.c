# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <curl/curl.h>
# include "../deps/cJSON.h"

struct memory_struct {
  char *memory;
  size_t size;
};

void set_base_endpoint (char **temp_endpoint, unsigned int endpoint_length) {
  // Check if BANDIERA_ENDPOINT has trailing '/' and add one if not
  // abort trap 6 possible here. Adding more data overflows the array
  char endpoint_addition[8] = "api/v2/\0";
  char *formatted_endpoint;

  // allocate memory for length of the endpoint and the endpoint_addition
  formatted_endpoint = (char *) malloc(endpoint_length + 7);

  // copy endpoint to the new larger array
  strcpy(formatted_endpoint, *temp_endpoint);

  if (formatted_endpoint[endpoint_length-1] != '/') {
    // concat '/' (as a string) onto the formatted endpoint
    strcat(formatted_endpoint, "/");
  }

  // Add the rest of the endpoint URI
  strcat(formatted_endpoint, endpoint_addition);

  // Change endpoint to point at formatted_endpoint
  *temp_endpoint = formatted_endpoint;
}

// Writes response and response size to memory_struct
static size_t write_memory_callback (void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct memory_struct *mem = (struct memory_struct *)userp;

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

// make the GET request
struct memory_struct make_request (char endpoint[]) {
  // cURL
  CURL *curl;
  CURLcode res;

  // Init curl session
  curl = curl_easy_init();

  struct memory_struct chunk;

  // set defaults
  chunk.memory = malloc(1);  // will be grown as needed by realloc in #write_memory_callback
  chunk.size = 0;    // no data at this point

  curl_global_init(CURL_GLOBAL_ALL);

  // Set URL to get
  curl_easy_setopt(curl, CURLOPT_URL, endpoint);

  // pass data to #write_memory_callback
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);

  // pass chunk struct to #write_memory_callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

  // set a user-agent
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

  // Make request
  res = curl_easy_perform(curl);

  // Check for errors
  if(res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
  }

  // Clean libcurl
  curl_easy_cleanup(curl);
  free(chunk.memory);
  curl_global_cleanup();

  return chunk;
}

int bandiera_is_enabled (char endpoint[], unsigned int endpoint_length, char group_name[], unsigned int group_name_length, char feature_name[], unsigned int feature_name_length) {
  const unsigned int length_of_additional_uri = strlen("groups//features/");

  // array length of full endpoint
  char new_endpoint[endpoint_length + group_name_length + feature_name_length + length_of_additional_uri];

  // Set double ref pointer var to endpoint
  char **temp_endpoint = &endpoint;

  // Create full base endpoint i.e. 'example.com/api/v2/'
  set_base_endpoint(temp_endpoint, endpoint_length);

  // create full endpoint for specific feature
  snprintf(new_endpoint, sizeof(new_endpoint) + length_of_additional_uri, "%sgroups/%s/features/%s", endpoint, group_name, feature_name);

  printf("ENDPOINT: %s\n", new_endpoint);

  struct memory_struct request_response;

  // make request and set response as memory_struct
  request_response = make_request(new_endpoint);

  cJSON *root_json_response = cJSON_Parse(request_response.memory);
  cJSON *feature_item = cJSON_GetObjectItemCaseSensitive(root_json_response, "response");

  printf("\n%i\n", feature_item->valueint);

  return feature_item->valueint;
}

int main (void)
{
  char endpoint[]     = "";
  char group_name[]   = "";
  char feature_name[] = "";

  unsigned int endpoint_length     = strlen(endpoint);
  unsigned int group_name_length   = strlen(group_name);
  unsigned int feature_name_length = strlen(feature_name);

  bandiera_is_enabled(endpoint, endpoint_length, group_name, group_name_length, feature_name, feature_name_length);

  return 0;
}
