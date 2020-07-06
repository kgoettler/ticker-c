#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <json-c/json.h>
#include <curl/curl.h>
#include "ticker.h"

int main (int argc, char **argv)
{
    json_object *json;
    json_object **pjson = &json;
    int res;

    /* Check inputs */
    if (argc == 1)
    {
        fprintf(stderr, "usage: ticker SYMBOL1 (SYMBOL2) ...\n");
        return 1;
    }

    char *full_url = build_full_url(BASE_URL, argc, argv);
    
    res = query(full_url, pjson);
   
    /* Print and clean */
    print_all_stocks(json);
    json_object_put(json);
    free(full_url);

    return 0;
};

char *build_full_url(const char *url, int nsymbols, char **symbols)
{
    /* Add argv to URL */
    int arglen = 0;
    for (int i = 1; i < nsymbols; i++)
        arglen += strlen(symbols[i]);

    char * fullurl = malloc(sizeof(char) * (arglen + strlen(url) + nsymbols));
    strcpy(fullurl, url);
    strcat(fullurl, symbols[1]);
    for (int i = 2; i < nsymbols; i++)
    {
        strcat(fullurl, ",");
        strcat(fullurl, symbols[i]);
    }

    return fullurl;
}

/*
 * Prints all stocks to the terminal. Calls print_stock on all of the
 * stocks in JSON object.
 *
 * @param jobj a json_object returned by the Yahoo Finance API
 */
void print_all_stocks(json_object *jobj)
{
    int len;
    json_object * jsub;
    jsub = json_object_object_get(jobj, "quoteResponse"); 
    jsub = json_object_object_get(jsub, "result");
    len = json_object_array_length(jsub);
    for (int i = 0; i < len; i++)
        print_stock(json_object_array_get_idx(jsub, i));
    return;
}

/*
 * Prints a single stock to the terminal.
 *
 * @param jobj pointer to a json_object
 */
void print_stock(json_object *jobj)
{
    const char *mstate, *symbol;
    char *msign;
    char color[12];
    double price, diff, percent, premc, postmc;
    symbol = json_object_get_string(
            json_object_object_get(jobj, "symbol"));
    mstate = json_object_get_string(
            json_object_object_get(jobj, "marketState"));
    premc = json_object_get_double(
            json_object_object_get(jobj, "preMarketChange"));
    postmc = json_object_get_double(
            json_object_object_get(jobj, "postMarketChange"));

    if (strcmp(mstate, "PRE") && (premc != 0))
    {
        msign = "*";
        price = json_object_get_double(
                json_object_object_get(jobj, "preMarketPrice"));
        diff = premc;
        percent = json_object_get_double(
                json_object_object_get(jobj, "preMarketChangePercent"));
    }
    else if (~strcmp(mstate, "REGULAR") && (postmc != 0))
    {
        msign = "*";
        price = json_object_get_double(
                json_object_object_get(jobj, "postMarketPrice"));
        diff = postmc;
        percent = json_object_get_double(
                json_object_object_get(jobj, "postMarketChangePercent"));
    }
    else
    {
        msign = " ";
        price = json_object_get_double(
                json_object_object_get(jobj, "regularMarketPrice"));
        diff = json_object_get_double(
                json_object_object_get(jobj, "regularMarketChange"));
        percent = json_object_get_double(
                json_object_object_get(jobj, "regularMarketChangePercent"));
    }

    if (diff == 0)
        strcpy(color, COLOR_RESET);
    else if (diff < 0)
        strcpy(color, COLOR_RED);
    else
        strcpy(color, COLOR_GREEN);

    printf("%-10s%s%8.2f%s", symbol, COLOR_BOLD, price, COLOR_RESET);
    printf("%s%10.2f%12.2lf%% %s", color, diff, percent, COLOR_RESET);
    printf("%s\n", msign);

    return;
}

/*
 * Query the Yahoo Finance API
 */

int query(char *url, json_object **json)
{
    CURL *ch;
    CURLcode rcode;
    struct curl_fetch_st curl_fetch;
    struct curl_fetch_st *cf = &curl_fetch;
    struct curl_slist *headers = NULL;
    enum json_tokener_error jerr = json_tokener_success;

    /* init curl handle */
    if ((ch = curl_easy_init()) == NULL)
    {
        fprintf(stderr, "ERROR: Failed to create curl handle in fetch_session");
        return 2;
    }

    /* set content type */
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");

    /* fetch page and capture return code */
    rcode = curl_fetch_url(ch, url, cf);

    /* cleanup curl */
    curl_easy_cleanup(ch);
    curl_slist_free_all(headers);

    /* check return code */
    if (rcode != CURLE_OK || cf->size < 1) 
    {
        fprintf(stderr, "ERROR: Failed to fetch url - curl said: %s",
            curl_easy_strerror(rcode));
        return 3;
    }

    /* check payload */
    if (cf->payload != NULL) 
    {
        *json = json_tokener_parse_verbose(cf->payload, &jerr);
        free(cf->payload);
    } 
    else 
    {
        fprintf(stderr, "ERROR: Failed to populate payload");
        free(cf->payload);
        return 4;
    }

    /* check error */
    if (jerr != json_tokener_success) 
    {
        fprintf(stderr, "ERROR: Failed to parse json string");
        json_object_put(*json);
        return 5;
    }
    return 0;
}

/*
 * Callback function for cURL call
 */
size_t curl_callback (void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct curl_fetch_st *p = (struct curl_fetch_st *) userp;

    /* expand buffer */
    p->payload = (char *) realloc(p->payload, p->size + realsize + 1);

    /* check buffer */
    if (p->payload == NULL) {
      fprintf(stderr, "ERROR: Failed to expand buffer in curl_callback");
      free(p->payload);
      return -1;
    }

    /* copy contents to buffer */
    memcpy(&(p->payload[p->size]), contents, realsize);

    /* set new buffer size */
    p->size += realsize;

    /* ensure null termination */
    p->payload[p->size] = 0;

    /* return size */
    return realsize;
}

CURLcode curl_fetch_url(CURL *ch, const char *url, struct curl_fetch_st *fetch) 
{
    CURLcode rcode;

    /* init payload */
    fetch->payload = (char *) calloc(1, sizeof(fetch->payload));

    /* check payload */
    if (fetch->payload == NULL) {
        /* log error */
        fprintf(stderr, "ERROR: Failed to allocate payload in curl_fetch_url");
        /* return error */
        return CURLE_FAILED_INIT;
    }

    /* init size */
    fetch->size = 0;

    /* set url to fetch */
    curl_easy_setopt(ch, CURLOPT_URL, url);

    /* set calback function */
    curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, curl_callback);

    /* pass fetch struct pointer */
    curl_easy_setopt(ch, CURLOPT_WRITEDATA, (void *) fetch);

    /* set default user agent */
    curl_easy_setopt(ch, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    /* set timeout */
    curl_easy_setopt(ch, CURLOPT_TIMEOUT, 5);

    /* enable location redirects */
    curl_easy_setopt(ch, CURLOPT_FOLLOWLOCATION, 1);

    /* set maximum allowed redirects */
    curl_easy_setopt(ch, CURLOPT_MAXREDIRS, 1);

    /* fetch the url */
    rcode = curl_easy_perform(ch);

    return rcode;
}
