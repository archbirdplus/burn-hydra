
SOURCES=burn_hydra.cpp basecase.cpp funnel.cpp chain.cpp
HEADERS=common.h basecase.h funnel.h chain.h

MPICC?=mpic++
CFLAGS+=-std=c++17

burn_hydra: ${SOURCES} ${HEADERS} out
	${MPICC} ${CFLAGS} -o out/burn_hydra ${SOURCES}

out:
	mkdir out

