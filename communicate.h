
#ifndef COMMUNICATE_H
#define COMMUNICATE_H

#include "common.h"

void sendLeft(segment_t*, int, mpz_t);
void receiveLeft(segment_t*, int, mpz_t);
void sendRight(segment_t*, int, mpz_t);
void receiveRight(segment_t*, int, mpz_t);

#endif COMMUNICATE_H

