
# Burn-Hydra

Burn-Hydra is a program that attempts to compute extremely large iterations of the [Hydra map](https://wiki.bbchallenge.org/wiki/Hydra_function). It has the capability of distributing the large integer computations over multiple processors to get around limitations of single node storage and GMP integer sizes.

The present goal is to compute the `H^114817814715809(10)`, which has the potential to resolve the halting problem for the Turing machine [`1RB1RE_1LC1LD_---1LA_1LB1LE_0RF0RA_1LD1RF`](https://wiki.bbchallenge.org/wiki/1RB1RE_1LC1LD_---1LA_1LB1LE_0RF0RA_1LD1RF). This will require at least 4 TB of storage, not accounting for temporary or auxiliary variables.

## Usage

Requirements:
- GMP
- MPI

Burn-Hydra depends on both GMP and an MPI implementation. Compilation looks something like this, but might need to be adapted for your system:
```
make
```

To use Burn-Hydra to compute the `$ITERATION`'th Hydra value strarting from 3, you will need a command like this:
```
mpirun -n $NUM_PROCESSORS -- out/burn_hydra --iterations $ITERATIONS --config '8-18,18-20,20-22,22-24,24-26,26-28/28-28-28' -x 3
```

The configuration string heavily impacts performance, so consider tuning it carefully. It is a comma-separated list of hyphen-separated tuples corresponding to the log-size of the blocks of integers each processor will be assigned.
For example, the string above tells the first processor to handle integer blocks of 2^8 bits and 2^18 bits, the next processor to handle blocks of 2^18 bits and 2^20 bits, and so on. The last section after the `/` tells each processors 7 and onwards to handle 3 blocks of 2^28 bits each.

