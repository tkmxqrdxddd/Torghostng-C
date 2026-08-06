#ifndef TORGHOSTNG_TOR_H
#define TORGHOSTNG_TOR_H

#include "core.h"

int tor_configure(TorGhostNG *tg, const char *exit_node);
int tor_restore_config(TorGhostNG *tg);
int tor_start_service(TorGhostNG *tg);
int tor_renew_circuit(void);

#endif /* TORGHOSTNG_TOR_H */
