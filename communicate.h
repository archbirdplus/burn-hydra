
#ifndef COMMUNICATE_H
#define COMMUNICATE_H

#include <gmp.h>
#include "common.h"

// might eventually need to pass a shift along with it
void sendLeft(segment_t*, mpz_t);
void receiveLeft(segment_t*, mpz_t);
void sendRight(segment_t*, mpz_t);
void receiveRight(segment_t*, mpz_t);

#endif // COMMUNICATE_H

