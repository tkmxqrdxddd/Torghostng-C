#include "torghostng.h"

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} Buffer;

static void curl_global_init_if_needed(void) {
    static int initialized = 0;
    if (!initialized) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        initialized = 1;
    }
}

static void buffer_free(Buffer *b) {
    free(b->data);
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

static size_t write_cb(char *ptr, size_t size, size_t nmemb, void *userdata) {
    Buffer *b = userdata;
    size_t n = size * nmemb;

    if (b->len + n + 1 > b->cap) {
        size_t new_cap = b->cap ? b->cap : 1024;
        while (b->len + n + 1 > new_cap) {
            new_cap *= 2;
        }
        char *new_data = realloc(b->data, new_cap);
        if (!new_data) {
            return 0;
        }
        b->data = new_data;
        b->cap = new_cap;
    }

    memcpy(b->data + b->len, ptr, n);
    b->len += n;
    b->data[b->len] = '\0';
    return n;
}

static CURL *curl_new(const char *url, Buffer *buf) {
    curl_global_init_if_needed();

    CURL *curl = curl_easy_init();
    if (!curl) {
        log_error("Failed to initialize curl");
        return NULL;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, buf);
    return curl;
}

static void curl_via_tor(CURL *curl) {
    char proxy[64];
    snprintf(proxy, sizeof(proxy), "socks5h://%s:%d", TOR_SOCKS_HOST, TOR_SOCKS_PORT);
    curl_easy_setopt(curl, CURLOPT_PROXY, proxy);
}

static const char *json_field(const char *json, const char *key) {
    char needle[64];
    snprintf(needle, sizeof(needle), "\"%s\"", key);

    const char *hit = strstr(json, needle);
    if (!hit) {
        return NULL;
    }

    const char *colon = strchr(hit, ':');
    if (!colon) {
        return NULL;
    }

    colon++;
    while (*colon == ' ' || *colon == '\t') {
        colon++;
    }
    return colon;
}

static const char *json_value(const char *token, char *out, size_t n) {
    if (*token == '"') {
        token++;
        size_t len = 0;
        while (token[len] != '\0' && token[len] != '"' && len + 1 < n) {
            out[len] = token[len];
            len++;
        }
        out[len] = '\0';
        return out;
    }

    size_t len = 0;
    while (token[len] != '\0' && token[len] != ',' && token[len] != '}' &&
           len + 1 < n) {
        out[len] = token[len];
        len++;
    }
    out[len] = '\0';
    return out;
}

int check_tor_connection(void) {
    log_message("Checking Tor connection...");

    Buffer buf = {0};
    CURL *curl = curl_new(TOR_CHECK_URL, &buf);
    if (!curl) {
        return 1;
    }
    curl_via_tor(curl);

    CURLcode res = curl_easy_perform(curl);
    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        log_error("Tor connection check failed: %s", curl_easy_strerror(res));
        buffer_free(&buf);
        return 1;
    }

    int using_tor = 0;
    const char *field = json_field(buf.data, "IsTor");
    if (field) {
        char value[32];
        using_tor = strcmp(json_value(field, value, sizeof(value)), "true") == 0;
    }

    if (response_code == 200 && using_tor) {
        const char *ip_field = json_field(buf.data, "IP");
        if (ip_field) {
            char ip[64];
            log_message("Tor is working - exit IP: %s",
                        json_value(ip_field, ip, sizeof(ip)));
        } else {
            log_message("Tor is working");
        }
        buffer_free(&buf);
        return 0;
    }

    log_error("Not using Tor (HTTP %ld)", response_code);
    buffer_free(&buf);
    return 1;
}

int check_ip(bool via_tor) {
    log_message("Checking IP address...");

    Buffer buf = {0};
    CURL *curl = curl_new(IP_CHECK_URL, &buf);
    if (!curl) {
        return 1;
    }
    if (via_tor) {
        curl_via_tor(curl);
    }

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        log_error("IP check failed: %s", curl_easy_strerror(res));
        buffer_free(&buf);
        return 1;
    }

    char *end = buf.data + buf.len;
    while (end > buf.data && (end[-1] == '\n' || end[-1] == '\r')) {
        end--;
    }
    *end = '\0';

    if (via_tor) {
        log_message("Current exit IP: %s", buf.data);
    } else {
        log_message("Current IP: %s", buf.data);
    }

    buffer_free(&buf);
    return 0;
}
