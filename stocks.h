#define COLOR_BOLD "\033[1;37m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_RESET "\033[0m"

struct curl_fetch_st {
    char *payload;
    size_t size;
};

size_t curl_callback (void *contents, size_t size, size_t nmemb, void *userp);
CURLcode curl_fetch_url(CURL *ch, const char *url, struct curl_fetch_st *fetch);
void print_stocks(json_object * jobj);
void print_stock(json_object * jobj);
