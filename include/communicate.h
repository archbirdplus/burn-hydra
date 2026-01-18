#pragma once

#include <gmp.h>
#include <flint/fmpz.h>

#include "state.h"

void send(Metrics*, int, int, fmpz_t);
void recv(Metrics*, int, int, fmpz_t);

// might eventually need to pass a shift along with it
void sendLeft(Context*, fmpz_t);
void receiveLeft(Context*, fmpz_t);
void sendRight(Context*, fmpz_t);
void receiveRight(Context*, fmpz_t);

void gather(Context*, fmpz_t, fmpz*, int);

