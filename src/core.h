#ifndef TORGHOSTNG_CORE_H
#define TORGHOSTNG_CORE_H

#include <stdbool.h>

typedef struct {
    bool ipv6_disabled;
    bool iptables_configured;
    bool dns_configured;
    char *dns_backup;
    bool torrc_modified;
    bool tor_service_started;
    char *exit_node;
} TorGhostNG;

void tg_init(TorGhostNG *tg);
void tg_cleanup(TorGhostNG *tg);

#endif /* TORGHOSTNG_CORE_H */
