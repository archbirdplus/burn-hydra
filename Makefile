

SOURCES=src/segment_burn.cpp src/segment_setups.cpp src/segment_results.cpp src/communicate.cpp src/metrics.cpp src/parse.cpp
BURN_SOURCES=src/burn_hydra.cpp
BENCH_SOURCES=src/bench.cpp
TEST_SOURCES=src/test.cpp
HEADERS=include/common.h include/segment.h include/communicate.h include/metrics.h include/parse.h

MPICC?=mpic++
CFLAGS+=-std=c++17 -lgmp -lstdc++ -I/opt/homebrew/Cellar/gmp/6.3.0/include -L/opt/homebrew/Cellar/gmp/6.3.0/lib -I include -Wall -Wextra

burn_hydra: ${SOURCES} ${HEADERS} out
	${MPICC} -g -O2 -o out/burn_hydra ${BURN_SOURCES} ${SOURCES} ${CFLAGS}

bench: ${TESTS} ${SOURCES} ${HEADERS} testdir
	${MPICC} -g -O2 -o testdir/bench ${BENCH_SOURCES} ${SOURCES} ${CFLAGS}
	./testdir/bench

test: test.test

test.test: ${TESTS} ${SOURCES} ${HEADERS} testdir
	${MPICC} -g -O0 -o testdir/test ${TEST_SOURCES} ${SOURCES} ${CFLAGS}
	./testdir/test

testdir:
	mkdir testdir

out:
	mkdir out

