#ifndef TORGHOSTNG_CHECK_H
#define TORGHOSTNG_CHECK_H

#include <stdbool.h>

int check_tor_connection(void);
int check_ip(bool via_tor);

#endif /* TORGHOSTNG_CHECK_H */
