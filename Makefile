
SOURCES=burn_hydra.cpp segment_burn.cpp communicate.cpp
HEADERS=common.h segment_burn.h communicate.h

MPICC?=mpic++
CFLAGS+=-std=c++17

burn_hydra: ${SOURCES} ${HEADERS} out
	${MPICC} ${CFLAGS} -o out/burn_hydra ${SOURCES}

out:
	mkdir out

