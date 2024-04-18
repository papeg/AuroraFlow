#include <mpi.h>

#include "collectives.hpp"
#include "auroraemu.hpp"

#include "offload.hpp"
#include "test.hpp"

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    MPI_Finalize();
}