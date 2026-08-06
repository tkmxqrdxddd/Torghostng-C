#include "torghostng.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char TORRC_BLOCK_BEGIN[] = "## TORGHOSTNG-CONFIG-BEGIN\n";
static const char TORRC_BLOCK_END[] = "## TORGHOSTNG-CONFIG-END\n";

#define STR_(x) #x
#define STR(x) STR_(x)

static char *torrc_block(const char *exit_node) {
    const char *dnport = "DNSPort " TOR_SOCKS_HOST ":" STR(TOR_DNS_PORT) "\n";
    char exit_line[64];
    size_t extra = 0;

    if (exit_node) {
        extra = snprintf(exit_line, sizeof(exit_line),
                         "ExitNodes {%s}\nStrictNodes 1\n", exit_node);
    }

    size_t n = sizeof(TORRC_BLOCK_BEGIN) - 1 + strlen(dnport) + extra +
               sizeof(TORRC_BLOCK_END) - 1;
    char *block = malloc(n + 1);
    if (!block) {
        return NULL;
    }

    char *p = block;
    memcpy(p, TORRC_BLOCK_BEGIN, sizeof(TORRC_BLOCK_BEGIN) - 1);
    p += sizeof(TORRC_BLOCK_BEGIN) - 1;
    memcpy(p, dnport, strlen(dnport));
    p += strlen(dnport);
    if (exit_node) {
        memcpy(p, exit_line, extra);
        p += extra;
    }
    memcpy(p, TORRC_BLOCK_END, sizeof(TORRC_BLOCK_END) - 1);
    p += sizeof(TORRC_BLOCK_END) - 1;
    *p = '\0';

    return block;
}

static char *strip_block(char *content) {
    char *begin = strstr(content, TORRC_BLOCK_BEGIN);
    if (!begin) {
        return content;
    }

    char *end = strstr(begin, TORRC_BLOCK_END);
    if (!end) {
        return content;
    }
    end += sizeof(TORRC_BLOCK_END) - 1;

    memmove(begin, end, strlen(end) + 1);
    return content;
}

static char *apply_block(const char *content, const char *block) {
    size_t clen = strlen(content);
    size_t blen = strlen(block);
    char *out = malloc(clen + blen + 2);
    if (!out) {
        return NULL;
    }

    memcpy(out, content, clen);
    char *p = out + clen;
    if (clen > 0 && p[-1] != '\n') {
        *p++ = '\n';
    }
    memcpy(p, block, blen + 1);
    return out;
}

int tor_configure(TorGhostNG *tg, const char *exit_node) {
    if (!tg || tg->torrc_modified) {
        return 0;
    }

    log_message("Configuring Tor...");

    char *original = read_file_alloc(TOR_CONFIG_PATH);
    if (!original) {
        log_error("Cannot read " TOR_CONFIG_PATH);
        return -1;
    }
    if (access(TOR_CONFIG_PATH, W_OK) != 0) {
        log_error(TOR_CONFIG_PATH " is not writable");
        free(original);
        return -1;
    }

    char *block = torrc_block(exit_node);
    if (!block) {
        log_error("Out of memory building Tor configuration");
        free(original);
        return -1;
    }

    char *stripped = strip_block(original);
    char *out = apply_block(stripped, block);
    free(block);
    free(original);

    if (!out) {
        log_error("Out of memory building Tor configuration");
        return -1;
    }

    if (write_file_atomic(TOR_CONFIG_PATH, out) != 0) {
        log_error("Failed to write " TOR_CONFIG_PATH);
        free(out);
        return -1;
    }
    free(out);

    tg->torrc_modified = true;
    if (exit_node) {
        log_message("Tor exit node set to {%s} (StrictNodes 1)", exit_node);
    } else {
        log_message("Tor DNS resolution configured (DNSPort %d)", TOR_DNS_PORT);
    }
    return 0;
}

int tor_restore_config(TorGhostNG *tg) {
    if (!tg || !tg->torrc_modified) {
        return 0;
    }

    log_message("Restoring Tor configuration...");

    char *content = read_file_alloc(TOR_CONFIG_PATH);
    if (content) {
        char *stripped = strip_block(content);
        if (write_file_atomic(TOR_CONFIG_PATH, stripped) != 0) {
            log_error("Failed to restore " TOR_CONFIG_PATH);
        }
        free(content);
    }

    tg->torrc_modified = false;
    log_message("Tor configuration restored");
    return 0;
}

int tor_start_service(TorGhostNG *tg) {
    if (!tg || tg->tor_service_started) {
        return 0;
    }

    log_message("Starting Tor service...");

    if (run_command("systemctl start tor 2>/dev/null || "
                    "service tor start 2>/dev/null") != 0) {
        log_error("Failed to start Tor service");
        return -1;
    }

    tg->tor_service_started = true;
    sleep(2);
    log_message("Tor service started");
    return 0;
}

int tor_renew_circuit(void) {
    log_message("Renewing Tor circuit...");

    if (run_command("pkill -HUP -x tor 2>/dev/null") != 0) {
        log_error("Failed to signal Tor daemon (is Tor running?)");
        return -1;
    }

    sleep(2);
    log_message("Circuit renewal signal sent");

    return check_tor_connection();
}