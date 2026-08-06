#include "torghostng.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char *const IPV6_DISABLE_CMD =
    "sysctl -w net.ipv6.conf.all.disable_ipv6=1 "
    "net.ipv6.conf.default.disable_ipv6=1 "
    "net.ipv6.conf.lo.disable_ipv6=1 >/dev/null 2>&1";

static const char *const IPV6_ENABLE_CMD =
    "sysctl -w net.ipv6.conf.all.disable_ipv6=0 "
    "net.ipv6.conf.default.disable_ipv6=0 "
    "net.ipv6.conf.lo.disable_ipv6=0 >/dev/null 2>&1";

#define STR_(x) #x
#define STR(x) STR_(x)

static const char *const IPTABLES_RULES =
    "iptables -t nat -A " IPTABLES_CHAIN " -o lo -j RETURN\n"
    "iptables -t nat -A " IPTABLES_CHAIN " -p tcp --dport 22 -j RETURN\n"
    "iptables -t nat -A " IPTABLES_CHAIN " -p tcp --dport " STR(TOR_SOCKS_PORT) " -j RETURN\n"
    "iptables -t nat -A " IPTABLES_CHAIN " -p tcp --dport 53 -j DNAT --to-destination 127.0.0.1:" STR(TOR_DNS_PORT) "\n"
    "iptables -t nat -A " IPTABLES_CHAIN " -p tcp -j REDIRECT --to-ports " STR(TOR_SOCKS_PORT) "\n"
    "iptables -t nat -A " IPTABLES_CHAIN " -p udp --dport 53 -j DNAT --to-destination 127.0.0.1:" STR(TOR_DNS_PORT);

static const char *const IPTABLES_SETUP =
    "iptables -t nat -N " IPTABLES_CHAIN " 2>/dev/null; "
    "iptables -t nat -F " IPTABLES_CHAIN " 2>/dev/null; "
    "iptables -t nat -C OUTPUT -j " IPTABLES_CHAIN " 2>/dev/null || "
    "iptables -t nat -A OUTPUT -j " IPTABLES_CHAIN;

static const char *const IPTABLES_TEARDOWN =
    "iptables -t nat -D OUTPUT -j " IPTABLES_CHAIN " 2>/dev/null; "
    "iptables -t nat -F " IPTABLES_CHAIN " 2>/dev/null; "
    "iptables -t nat -X " IPTABLES_CHAIN " 2>/dev/null";

int net_disable_ipv6(TorGhostNG *tg) {
    if (!tg || tg->ipv6_disabled) {
        return 0;
    }

    log_message("Disabling IPv6...");
    if (run_command(IPV6_DISABLE_CMD) != 0) {
        log_error("Failed to disable IPv6 (continuing without it)");
        return -1;
    }

    tg->ipv6_disabled = true;
    log_message("IPv6 disabled");
    return 0;
}

int net_restore_ipv6(TorGhostNG *tg) {
    if (!tg || !tg->ipv6_disabled) {
        return 0;
    }

    if (run_command(IPV6_ENABLE_CMD) != 0) {
        log_error("Failed to restore IPv6 settings");
    }

    tg->ipv6_disabled = false;
    log_message("IPv6 restored");
    return 0;
}

int net_setup_iptables(TorGhostNG *tg) {
    if (!tg || tg->iptables_configured) {
        return 0;
    }

    log_message("Configuring iptables...");

    if (run_command(IPTABLES_SETUP) != 0) {
        log_error("Failed to create iptables chain " IPTABLES_CHAIN);
        return -1;
    }

    if (run_command(IPTABLES_RULES) != 0) {
        log_error("Failed to apply iptables rules");
        net_teardown_iptables(tg);
        return -1;
    }

    tg->iptables_configured = true;
    log_message("iptables rules applied via chain " IPTABLES_CHAIN);
    return 0;
}

int net_teardown_iptables(TorGhostNG *tg) {
    if (!tg || !tg->iptables_configured) {
        return 0;
    }

    log_message("Removing iptables chain " IPTABLES_CHAIN "...");

    if (run_command(IPTABLES_TEARDOWN) != 0) {
        log_error("Failed to remove iptables chain " IPTABLES_CHAIN);
    }

    tg->iptables_configured = false;
    log_message("iptables rules removed");
    return 0;
}

int net_configure_dns(TorGhostNG *tg) {
    if (!tg || tg->dns_configured) {
        return 0;
    }

    log_message("Configuring DNS...");

    char *backup = read_file_alloc(RESOLV_CONF_PATH);
    if (!backup) {
        log_error("Could not read " RESOLV_CONF_PATH);
        return -1;
    }

    char buf[128];
    snprintf(buf, sizeof(buf), "# Managed by torghostng (v%s)\nnameserver %s\n",
             VERSION, DNS_NAMESERVER);

    if (write_file(RESOLV_CONF_PATH, buf) != 0) {
        log_error("Failed to modify " RESOLV_CONF_PATH);
        free(backup);
        return -1;
    }

    tg->dns_backup = backup;
    tg->dns_configured = true;
    log_message("DNS routed via Tor (nameserver %s -> DNSPort %d)", DNS_NAMESERVER, TOR_DNS_PORT);
    return 0;
}

int net_restore_dns(TorGhostNG *tg) {
    if (!tg || !tg->dns_configured) {
        return 0;
    }

    log_message("Restoring DNS configuration...");

    if (tg->dns_backup && write_file(RESOLV_CONF_PATH, tg->dns_backup) != 0) {
        log_error("Failed to restore " RESOLV_CONF_PATH);
    }

    free(tg->dns_backup);
    tg->dns_backup = NULL;
    tg->dns_configured = false;
    log_message("DNS configuration restored");
    return 0;
}

int net_restart_network(void) {
    log_message("Restarting network services...");

    if (run_command("systemctl restart networking 2>/dev/null || "
                    "service networking restart 2>/dev/null || "
                    "systemctl restart NetworkManager 2>/dev/null; true") != 0) {
        log_error("Failed to restart network services");
        return -1;
    }

    log_message("Network services restarted");
    return 0;
}
