#ifndef TORGHOSTNG_H
#define TORGHOSTNG_H

#define _POSIX_C_SOURCE 200809L

#include <stdarg.h>
#include <stdbool.h>

#define VERSION "1.1.0"

#define TOR_CONFIG_PATH "/etc/tor/torrc"
#define RESOLV_CONF_PATH "/etc/resolv.conf"
#define DNS_NAMESERVER "1.1.1.1"

#define TOR_SOCKS_HOST "127.0.0.1"
#define TOR_SOCKS_PORT 9050
#define TOR_DNS_PORT 5353
#define IPTABLES_CHAIN "TORGHOSTNG"

#define IP_CHECK_URL "https://api.ipify.org"
#define TOR_CHECK_URL "https://check.torproject.org/api/ip"

#include "core.h"
#include "util.h"
#include "net.h"
#include "tor.h"
#include "check.h"

#endif /* TORGHOSTNG_H */
