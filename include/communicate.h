#pragma once

#include <gmp.h>
#include <flint/fmpz.h>

#include "state.h"

void send(Metrics*, int, int, fmpz_t);
void recv(Metrics*, int, int, fmpz_t);

void sendLeft(Context*, fmpz_t);
void receiveLeft(Context*, fmpz_t);
void sendRight(Context*, fmpz_t);
void receiveRight(Context*, fmpz_t);

void sendLeft(Context*, timed_fmpz&);
void receiveLeft(Context*, timed_fmpz&);
void sendRight(Context*, timed_fmpz&);
void receiveRight(Context*, timed_fmpz&);

void gather(Context*, fmpz_t, fmpz*, int);

