#ifndef TICKER_H
#define TICKER_H

#define COLOR_BOLD "\033[1;37m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_RESET "\033[0m"
#define BASE_URL "https://query1.finance.yahoo.com/v7/finance/quote?lang=en-US&region=US&corsDomain=finance.yahoo.com&fields=shortName,symbol,marketState,regularMarketPrice,regularMarketChange,regularMarketChangePercent,preMarketPrice,preMarketChange,preMarketChangePercent,postMarketPrice,postMarketChange,postMarketChangePercent&symbols="

struct curl_fetch_st {
    char *payload;
    size_t size;
};

char *build_full_url(const char *url, int nsymbols, char **symbols);
int query(char *url, json_object **json);
void print_all_stocks(json_object *jobj);
void print_stock(json_object *jobj);
size_t curl_callback (void *contents, size_t size, size_t nmemb, void *userp);
CURLcode curl_fetch_url(CURL *ch, const char *url, struct curl_fetch_st *fetch);

#endif
