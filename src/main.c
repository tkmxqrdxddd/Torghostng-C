#include "torghostng.h"

#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t g_interrupted = 0;

static void on_signal(int sig) {
    (void)sig;
    g_interrupted = 1;
}

static void print_usage(void) {
    printf("TorGhostNG v%s - Tor Proxy Management Tool\n", VERSION);
    printf("\n");
    printf("Usage: torghostng [OPTION] [ARG]\n");
    printf("\n");
    printf("Options:\n");
    printf("  -s, --start [EXIT_NODE]  Start Tor proxy (optional: 2-letter country code)\n");
    printf("  -x, --stop               Stop Tor proxy and restore system settings\n");
    printf("  -r, --renew              Renew Tor circuit\n");
    printf("  -c, --check              Check Tor connection and exit IP\n");
    printf("  -h, --help               Display this help message\n");
    printf("  -v, --version            Display version information\n");
    printf("\n");
    printf("Examples:\n");
    printf("  torghostng --start              Start with default exit node\n");
    printf("  torghostng --start DE           Start with German exit node\n");
    printf("  torghostng --renew              Renew Tor circuit\n");
    printf("\n");
    printf("Note: This program must be run as root for full functionality.\n");
}

static void print_version(void) {
    printf("TorGhostNG version %s\n", VERSION);
}

typedef enum {
    ACTION_START,
    ACTION_STOP,
    ACTION_RENEW,
    ACTION_CHECK,
    ACTION_HELP,
    ACTION_VERSION,
    ACTION_NONE
} Action;

static const struct {
    const char *name;
    Action action;
    bool takes_optional_arg;
} OPTIONS[] = {
    {"-s", ACTION_START, true},
    {"--start", ACTION_START, true},
    {"-x", ACTION_STOP, false},
    {"--stop", ACTION_STOP, false},
    {"-r", ACTION_RENEW, false},
    {"--renew", ACTION_RENEW, false},
    {"-c", ACTION_CHECK, false},
    {"--check", ACTION_CHECK, false},
    {"-h", ACTION_HELP, false},
    {"--help", ACTION_HELP, false},
    {"-v", ACTION_VERSION, false},
    {"--version", ACTION_VERSION, false},
};

static Action parse_action(const char *arg, bool *takes_arg) {
    for (size_t i = 0; i < sizeof(OPTIONS) / sizeof(OPTIONS[0]); i++) {
        if (strcmp(arg, OPTIONS[i].name) == 0) {
            *takes_arg = OPTIONS[i].takes_optional_arg;
            return OPTIONS[i].action;
        }
    }
    return ACTION_NONE;
}

static int normalize_exit_node(const char *raw, char *out, size_t n) {
    if (strlen(raw) != 2 || !isalpha((unsigned char)raw[0]) ||
        !isalpha((unsigned char)raw[1]) || n < 3) {
        return -1;
    }
    out[0] = (char)toupper((unsigned char)raw[0]);
    out[1] = (char)toupper((unsigned char)raw[1]);
    out[2] = '\0';
    return 0;
}

static void rollback_all(TorGhostNG *tg) {
    if (tg->torrc_modified) {
        tor_restore_config(tg);
    }
    if (tg->dns_configured) {
        net_restore_dns(tg);
    }
    if (tg->iptables_configured) {
        net_teardown_iptables(tg);
    }
    net_restore_ipv6(tg);
}

static int cmd_start(const char *exit_node) {
    TorGhostNG tg;
    tg_init(&tg);

    int rc = 0;

    if (g_interrupted) {
        rc = 130;
        goto out;
    }

    net_disable_ipv6(&tg);

    if (g_interrupted || tor_configure(&tg, exit_node) != 0) {
        rc = g_interrupted ? 130 : 1;
        goto rollback;
    }

    if (g_interrupted || net_configure_dns(&tg) != 0) {
        rc = g_interrupted ? 130 : 1;
        goto rollback;
    }

    if (g_interrupted || tor_start_service(&tg) != 0) {
        rc = g_interrupted ? 130 : 1;
        goto rollback;
    }

    if (g_interrupted || net_setup_iptables(&tg) != 0) {
        rc = g_interrupted ? 130 : 1;
        goto rollback;
    }

    if (g_interrupted) {
        rc = 130;
        goto rollback;
    }

    if (check_tor_connection() != 0) {
        log_error("Tor is not reachable; settings applied but connection "
                  "verification failed");
        rc = 1;
        goto rollback;
    }

    log_message("TorGhostNG started successfully");
    goto out;

rollback:
    if (rc != 130) {
        log_error("Startup failed, rolling back...");
    }
    rollback_all(&tg);
    log_message("TorGhostNG rollback complete");

out:
    tg_cleanup(&tg);
    return rc;
}

static int cmd_stop(void) {
    TorGhostNG tg;
    tg_init(&tg);

    log_message("Stopping TorGhostNG...");

    net_restore_dns(&tg);
    net_teardown_iptables(&tg);
    net_restore_ipv6(&tg);
    tor_restore_config(&tg);
    net_restart_network();

    log_message("TorGhostNG stopped (Tor service left running)");

    tg_cleanup(&tg);
    return 0;
}

static int cmd_renew(void) {
    if (tor_renew_circuit() != 0) {
        log_error("Circuit renewal failed");
        return 1;
    }
    return 0;
}

static int cmd_check(void) {
    int rc = 0;
    if (check_tor_connection() != 0) {
        rc = 1;
    }
    check_ip(true);
    return rc;
}

int main(int argc, char *argv[]) {
    if (geteuid() != 0) {
        fprintf(stderr,
                "Warning: This program should be run as root for full "
                "functionality.\n");
    }

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    if (argc < 2) {
        print_usage();
        return EXIT_FAILURE;
    }

    bool takes_optional_arg = false;
    Action action = parse_action(argv[1], &takes_optional_arg);
    if (action == ACTION_NONE) {
        fprintf(stderr, "Unknown option: %s\n", argv[1]);
        print_usage();
        return 2;
    }

    char exit_node[3] = {0};
    if (takes_optional_arg && argc > 2) {
        if (normalize_exit_node(argv[2], exit_node, sizeof(exit_node)) != 0) {
            fprintf(stderr, "Invalid exit node: %s (expected a 2-letter "
                            "country code, e.g. DE, US)\n",
                    argv[2]);
            print_usage();
            return 2;
        }
        if (argc > 3) {
            fprintf(stderr, "Too many arguments for: %s\n", argv[1]);
            print_usage();
            return 2;
        }
    }

    switch (action) {
    case ACTION_START:
        return cmd_start(exit_node[0] ? exit_node : NULL);
    case ACTION_STOP:
        return cmd_stop();
    case ACTION_RENEW:
        return cmd_renew();
    case ACTION_CHECK:
        return cmd_check();
    case ACTION_HELP:
        print_usage();
        return EXIT_SUCCESS;
    case ACTION_VERSION:
        print_version();
        return EXIT_SUCCESS;
    default:
        return 2;
    }
}
