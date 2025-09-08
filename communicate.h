
#ifndef COMMUNICATE_H
#define COMMUNICATE_H

#include <gmp.h>
#include "common.h"
#include "segment.h"

// might eventually need to pass a shift along with it
void sendLeft(data_t*, mpz_t);
void receiveLeft(data_t*, mpz_t);
void sendRight(data_t*, mpz_t);
void receiveRight(data_t*, mpz_t);

void gather(data_t*, mpz_t, mpz_ptr*, int);

#endif // COMMUNICATE_H

