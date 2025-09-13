
#ifndef COMMUNICATE_H
#define COMMUNICATE_H

#include <gmp.h>
#include <flint/fmpz.h>
#include "common.h"
#include "segment.h"

// might eventually need to pass a shift along with it
void sendLeft(data_t*, fmpz_t);
void receiveLeft(data_t*, fmpz_t);
void sendRight(data_t*, fmpz_t);
void receiveRight(data_t*, fmpz_t);

void gather(data_t*, fmpz_t, fmpz*, int);

#endif // COMMUNICATE_H

