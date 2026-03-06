#pragma once

#include <gmp.h>
#include <flint/fmpz.h>

#include "state.h"

void send(Metrics*, int, int, fmpz_t);
void recv(Metrics*, int, int, fmpz_t);

void sendLeft(Metrics*, int, fmpz_t);
void receiveLeft(Metrics*, int, fmpz_t);
void sendRight(Metrics*, int, fmpz_t);
void receiveRight(Metrics*, int, fmpz_t);

void sendLeft(Metrics*, int, timed_fmpz*);
void receiveLeft(Metrics*, int, timed_fmpz*);
void sendRight(Metrics*, int, timed_fmpz*);
void receiveRight(Metrics*, int, timed_fmpz*);

void gather(Context*, fmpz_t, fmpz*, int);

