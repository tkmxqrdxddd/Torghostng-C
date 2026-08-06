#ifndef TORGHOSTNG_NET_H
#define TORGHOSTNG_NET_H

#include "core.h"

int net_disable_ipv6(TorGhostNG *tg);
int net_restore_ipv6(TorGhostNG *tg);
int net_setup_iptables(TorGhostNG *tg);
int net_teardown_iptables(TorGhostNG *tg);
int net_configure_dns(TorGhostNG *tg);
int net_restore_dns(TorGhostNG *tg);
int net_restart_network(void);

#endif /* TORGHOSTNG_NET_H */
