

SOURCES=src/segment_burn.cpp src/segment_setups.cpp src/segment_results.cpp src/communicate.cpp src/metrics.cpp src/parse.cpp
BURN_SOURCES=src/burn_hydra.cpp
BENCH_SOURCES=src/bench.cpp
TEST_SOURCES=src/test.cpp
HEADERS=include/common.h include/segment.h include/communicate.h include/metrics.h include/parse.h

MPICC?=mpic++
CFLAGS+=-std=c++17 -lstdc++ -L/opt/homebrew/Cellar/flint/3.3.1/lib -L/opt/homebrew/Cellar/gmp/6.3.0/lib -I/opt/homebrew/Cellar/flint/3.3.1/include -I/opt/homebrew/Cellar/gmp/6.3.0/include -lflint -lgmp -I include -Wall -Wextra

out/burn_hydra: ${SOURCES} ${HEADERS} ${BURN_SOURCES} out
	${MPICC} -g -O2 -o out/burn_hydra ${BURN_SOURCES} ${SOURCES} ${CFLAGS}

bench: bench_basecase bench_smallchain

testdir/burn_hydra: ${TESTS} ${SOURCES} ${HEADERS} ${BURN_SOURCES} testdir
	${MPICC} -g -O2 -o testdir/burn_hydra ${BURN_SOURCES} ${SOURCES} ${CFLAGS} -DNO_PLOT_LOGS

# a benchmark heavily bottlenecked by medium-integer performance
bench_smallchain: testdir/burn_hydra
	echo "Benchmark: oversized funnel multiplication"
	for i in $$(seq 3); do \
		(time (mpirun -n 2 -- testdir/burn_hydra -x 8 -n 67108864 -c 8-18,18-26 \
			| grep 31848934250314775156605172273469025153 | grep 2246674935863200705435021934434735832940657095564955971137014 \
			|| echo "Incorrect signature.") ) 2>&1 | grep -e real -e 'H^2^' -e "Incorrect"; \
	done

testdir/bench: ${TESTS} ${SOURCES} ${HEADERS} ${BENCH_SOURCES} testdir
	${MPICC} -g -O2 -o testdir/bench ${BENCH_SOURCES} ${SOURCES} ${CFLAGS}

# a test consisting only of the lowest few bits
bench_basecase: testdir/bench
	echo "Benchmark: just basecase"
	./testdir/bench

test: test.test

test.test: ${TESTS} ${SOURCES} ${HEADERS} ${TEST_SOURCES} testdir
	${MPICC} -g -O0 -o testdir/test ${TEST_SOURCES} ${SOURCES} ${CFLAGS}
	./testdir/test

testdir:
	mkdir testdir

out:
	mkdir out

