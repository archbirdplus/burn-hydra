#ifndef PARSE_H
#define PARSE_H

#include "common.h"

void parse_args(problem_t* problem, config_t* config, int argc, char** argv);

void test_parse_config();
void test_parse_args();

#endif // PARSE_H

