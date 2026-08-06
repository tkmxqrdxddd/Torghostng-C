#include "torghostng.h"

#include <stdlib.h>
#include <string.h>

void tg_init(TorGhostNG *tg) {
    if (!tg) {
        return;
    }
    memset(tg, 0, sizeof(*tg));
}

void tg_cleanup(TorGhostNG *tg) {
    if (!tg) {
        return;
    }

    free(tg->dns_backup);
    tg->dns_backup = NULL;
    free(tg->exit_node);
    tg->exit_node = NULL;
}
