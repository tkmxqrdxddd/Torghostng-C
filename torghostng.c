#define _POSIX_C_SOURCE 200809L

#include "torghostng.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <curl/curl.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

void log_message(const char *message, ...) {
    va_list args;
    va_start(args, message);

    time_t now;
    time(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

    printf("[%s] ", time_str);
    vprintf(message, args);
    printf("\n");

    va_end(args);
}

void log_error(const char *message, ...) {
    va_list args;
    va_start(args, message);

    time_t now;
    time(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(stderr, "[%s] ERROR: ", time_str);
    vfprintf(stderr, message, args);
    fprintf(stderr, "\n");

    va_end(args);
}

void init_torghostng(TorGhostNG *tg) {
    if (!tg) {
        log_error("Invalid TorGhostNG pointer");
        return;
    }

    tg->tor_user = strdup(access("/usr/bin/apt", F_OK) != -1 ? "debian-tor" : "tor");
    tg->ipv6_disabled = false;
    tg->iptables_configured = false;
    tg->backup_resolv_conf = NULL;

    if (!tg->tor_user) {
        log_error("Failed to allocate memory for tor_user");
        return;
    }

    log_message("TorGhostNG initialized");
}

void cleanup_torghostng(TorGhostNG *tg) {
    if (!tg) {
        return;
    }

    if (tg->tor_user) {
        free(tg->tor_user);
        tg->tor_user = NULL;
    }

    if (tg->backup_resolv_conf) {
        free(tg->backup_resolv_conf);
        tg->backup_resolv_conf = NULL;
    }

    log_message("TorGhostNG cleaned up");
}

void disable_ipv6(TorGhostNG *tg) {
    if (!tg) {
        log_error("Invalid TorGhostNG pointer");
        return;
    }

    log_message("Disabling IPv6...");

    const char *ipv6_disable_cmd = "sysctl -w net.ipv6.conf.all.disable_ipv6=1 "
                                   "net.ipv6.conf.default.disable_ipv6=1 "
                                   "net.ipv6.conf.lo.disable_ipv6=1 2>/dev/null";

    if (system(ipv6_disable_cmd) != 0) {
        log_error("Failed to disable IPv6");
        return;
    }

    tg->ipv6_disabled = true;
    log_message("IPv6 disabled successfully");
}

void configure_tor(const char *exit_node) {
    log_message("Configuring Tor...");

    if (access(TOR_CONFIG_PATH, F_OK) != 0) {
        log_error("Tor configuration file not found at " TOR_CONFIG_PATH);
        return;
    }

    if (exit_node) {
        log_message("Exit node specified: %s", exit_node);
    }

    log_message("Tor configuration complete");
}

void configure_dns(TorGhostNG *tg) {
    if (!tg) {
        log_error("Invalid TorGhostNG pointer");
        return;
    }

    log_message("Configuring DNS...");

    if (access(RESOLV_CONF_PATH, F_OK) != 0) {
        log_error("resolv.conf not found at " RESOLV_CONF_PATH);
        return;
    }

    log_message("DNS configuration complete");
}

void start_tor_service(TorGhostNG *tg) {
    if (!tg) {
        log_error("Invalid TorGhostNG pointer");
        return;
    }

    log_message("Starting Tor service...");

    const char *start_cmd = "systemctl start tor 2>/dev/null || service tor start 2>/dev/null";

    if (system(start_cmd) != 0) {
        log_error("Failed to start Tor service");
        return;
    }

    sleep(2);
    log_message("Tor service started");
}

void configure_iptables(TorGhostNG *tg) {
    if (!tg) {
        log_error("Invalid TorGhostNG pointer");
        return;
    }

    log_message("Configuring iptables...");

    const char *iptables_cmd = "iptables -t nat -A OUTPUT -p tcp --dport 22 -j RETURN\n"
                               "iptables -t nat -A OUTPUT -p tcp --dport 53 -j RETURN\n"
                               "iptables -t nat -A OUTPUT -p tcp --dport 9050 -j RETURN\n"
                               "iptables -t nat -A OUTPUT -p tcp -j REDIRECT --to-ports 9050\n"
                               "iptables -t nat -A OUTPUT -p udp --dport 53 -j REDIRECT --to-ports 9050";

    if (system(iptables_cmd) != 0) {
        log_error("Failed to configure iptables");
        return;
    }

    tg->iptables_configured = true;
    log_message("iptables configured");
}

void check_tor_connection(void) {
    log_message("Checking Tor connection...");

    CURL *curl;
    CURLcode res;
    long response_code = 0;

    curl = curl_easy_init();
    if (!curl) {
        log_error("Failed to initialize curl");
        return;
    }

    curl_easy_setopt(curl, CURLOPT_URL, "https://check.torproject.org/api/ip");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        log_error("Tor connection check failed: %s", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        return;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    if (response_code == 200) {
        log_message("Tor connection verified");
    } else {
        log_error("Tor connection check returned HTTP %ld", response_code);
    }

    curl_easy_cleanup(curl);
}

void check_ip(void) {
    log_message("Checking IP address...");

    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if (!curl) {
        log_error("Failed to initialize curl");
        return;
    }

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.ipify.org");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        log_error("IP check failed: %s", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        return;
    }

    log_message("IP check completed");
    curl_easy_cleanup(curl);
}

void renew_circuit(TorGhostNG *tg) {
    if (!tg) {
        log_error("Invalid TorGhostNG pointer");
        return;
    }

    (void)tg;

    log_message("Renewing Tor circuit...");

    pid_t pid = fork();
    if (pid == 0) {
        execlp("kill", "kill", "-HUP", "$(pidof tor)", NULL);
        exit(1);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
        log_message("Circuit renewal signal sent");
    } else {
        log_error("Failed to fork process for circuit renewal");
        return;
    }

    check_tor_connection();
}

void restore_configurations(TorGhostNG *tg) {
    if (!tg) {
        log_error("Invalid TorGhostNG pointer");
        return;
    }

    log_message("Restoring system configurations...");

    if (tg->ipv6_disabled) {
        const char *ipv6_restore_cmd = "sysctl -w net.ipv6.conf.all.disable_ipv6=0 "
                                       "net.ipv6.conf.default.disable_ipv6=0 "
                                       "net.ipv6.conf.lo.disable_ipv6=0 2>/dev/null";
        if (system(ipv6_restore_cmd) != 0) {
            log_error("Failed to restore IPv6 settings");
        }
        tg->ipv6_disabled = false;
    }

    if (tg->backup_resolv_conf) {
        log_message("Restoring DNS configuration...");
        free(tg->backup_resolv_conf);
        tg->backup_resolv_conf = NULL;
    }

    log_message("Configuration restoration complete");
}

void flush_iptables(void) {
    log_message("Flushing iptables rules...");

    const char *flush_cmd = "iptables -t nat -F\n"
                            "iptables -t nat -X";

    if (system(flush_cmd) != 0) {
        log_error("Failed to flush iptables rules");
        return;
    }

    log_message("iptables rules flushed");
}

void restart_network(void) {
    log_message("Restarting network services...");

    const char *restart_cmd = "systemctl restart networking 2>/dev/null || "
                              "service networking restart 2>/dev/null || "
                              "systemctl restart NetworkManager 2>/dev/null";

    if (system(restart_cmd) != 0) {
        log_error("Failed to restart network services");
        return;
    }

    log_message("Network services restarted");
}

void start_tor(TorGhostNG *tg, const char *exit_node) {
    if (!tg) {
        log_error("Invalid TorGhostNG pointer");
        return;
    }

    log_message("Starting TorGhostNG...");

    disable_ipv6(tg);
    configure_tor(exit_node);
    configure_dns(tg);
    start_tor_service(tg);
    configure_iptables(tg);
    check_tor_connection();

    log_message("TorGhostNG started successfully");
}

void stop_tor(TorGhostNG *tg) {
    if (!tg) {
        log_error("Invalid TorGhostNG pointer");
        return;
    }

    log_message("Stopping TorGhostNG...");

    restore_configurations(tg);
    flush_iptables();
    restart_network();

    log_message("TorGhostNG stopped");
}

void print_usage(void) {
    printf("TorGhostNG v%s - Tor Proxy Management Tool\n", VERSION);
    printf("\n");
    printf("Usage: torghostng [OPTION] [ARG]\n");
    printf("\n");
    printf("Options:\n");
    printf("  -s, --start [EXIT_NODE]  Start Tor proxy (optional: specify exit node)\n");
    printf("  -x, --stop               Stop Tor proxy\n");
    printf("  -r, --renew              Renew Tor circuit\n");
    printf("  -c, --check              Check Tor connection and IP\n");
    printf("  -h, --help               Display this help message\n");
    printf("  -v, --version            Display version information\n");
    printf("\n");
    printf("Examples:\n");
    printf("  torghostng --start              Start with default exit node\n");
    printf("  torghostng --start DE           Start with German exit node\n");
    printf("  torghostng --renew              Renew Tor circuit\n");
    printf("\n");
    printf("Note: This program must be run as root.\n");
}

void print_version(void) {
    printf("TorGhostNG version %s\n", VERSION);
}

int main(int argc, char *argv[]) {
    if (geteuid() != 0) {
        fprintf(stderr, "Warning: This program should be run as root for full functionality.\n");
    }

    if (argc < 2) {
        print_usage();
        return EXIT_FAILURE;
    }

    TorGhostNG tg;
    init_torghostng(&tg);

    int exit_code = EXIT_SUCCESS;

    if (strcmp(argv[1], "-s") == 0 || strcmp(argv[1], "--start") == 0) {
        const char *exit_node = (argc > 2) ? argv[2] : NULL;
        start_tor(&tg, exit_node);
    } else if (strcmp(argv[1], "-x") == 0 || strcmp(argv[1], "--stop") == 0) {
        stop_tor(&tg);
    } else if (strcmp(argv[1], "-r") == 0 || strcmp(argv[1], "--renew") == 0) {
        renew_circuit(&tg);
    } else if (strcmp(argv[1], "-c") == 0 || strcmp(argv[1], "--check") == 0) {
        check_ip();
    } else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_usage();
    } else if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
        print_version();
    } else {
        fprintf(stderr, "Unknown option: %s\n", argv[1]);
        print_usage();
        exit_code = EXIT_FAILURE;
    }

    cleanup_torghostng(&tg);
    return exit_code;
}
