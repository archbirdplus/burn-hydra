
#include "parse.h"
#include "latencies.h"
#include "state.h"

int main() {
    test_parse_config();
    test_parse_args();
    test_get_opponent();
    test_fluent();
    return 0;
}

