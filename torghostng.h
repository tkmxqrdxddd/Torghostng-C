#ifndef TORGHOSTNG_H
#define TORGHOSTNG_H

#include <stdbool.h>
#include <stdarg.h>

#define TOR_CONFIG_PATH "/etc/tor/torrc"
#define RESOLV_CONF_PATH "/etc/resolv.conf"
#define SYSCTL_CONF_PATH "/etc/sysctl.conf"
#define LOG_FILE "torghostng.log"
#define VERSION "1.0.0"

typedef struct {
    char *tor_user;
    bool ipv6_disabled;
    bool iptables_configured;
    char *backup_resolv_conf;
} TorGhostNG;

void init_torghostng(TorGhostNG *tg);
void cleanup_torghostng(TorGhostNG *tg);

void start_tor(TorGhostNG *tg, const char *exit_node);
void stop_tor(TorGhostNG *tg);
void renew_circuit(TorGhostNG *tg);
void check_ip(void);

void disable_ipv6(TorGhostNG *tg);
void configure_tor(const char *exit_node);
void configure_dns(TorGhostNG *tg);
void start_tor_service(TorGhostNG *tg);
void configure_iptables(TorGhostNG *tg);
void check_tor_connection(void);

void restore_configurations(TorGhostNG *tg);
void flush_iptables(void);
void restart_network(void);

void log_message(const char *message, ...);
void log_error(const char *message, ...);
void print_usage(void);
void print_version(void);

#endif /* TORGHOSTNG_H */
