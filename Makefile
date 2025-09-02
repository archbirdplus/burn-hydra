
SOURCES=burn_hydra.cpp segment_burn.cpp segment_results.cpp communicate.cpp
HEADERS=common.h segment.h communicate.h

MPICC?=mpic++
CFLAGS+=-std=c++17 -lgmp -lgmpxx -I/opt/homebrew/Cellar/gmp/6.3.0/include -L/opt/homebrew/Cellar/gmp/6.3.0/lib

burn_hydra: ${SOURCES} ${HEADERS} out
	${MPICC} ${CFLAGS} -g -o out/burn_hydra ${SOURCES}

out:
	mkdir out

