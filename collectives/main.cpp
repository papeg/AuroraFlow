#include <mpi.h>
#include <thread>

#include "collectives.hpp"
#include "auroraemu.hpp"

#include "offload.hpp"
#include "test.hpp"

inline std::string stream_id(std::string prefix, size_t rank)
{
    std::stringstream identifier;
    identifier << prefix << "_" << rank;
    return identifier.str();
}

inline std::string aurora_id(size_t rank, size_t port)
{
    std::stringstream identifier;
    identifier << "a_" << rank << "_" << port;
    return identifier.str();
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD , &rank);
    MPI_Comm_size(MPI_COMM_WORLD , &size);

    std::optional<AuroraEmuSwitch> aurora_switch;
    if (rank == 0)
    {
        aurora_switch.emplace("127.0.0.1", 20000);
    }
    STREAM<stream_word> ring_0_in(stream_id("ring_0_in", rank).c_str());
    STREAM<stream_word> ring_0_out(stream_id("ring_0_out", rank).c_str());
    STREAM<stream_word> ring_1_in(stream_id("ring_1_in", rank).c_str());
    STREAM<stream_word> ring_1_out(stream_id("ring_1_out", rank).c_str());
    STREAM<stream_word> offload_in(stream_id("offload_in", rank).c_str());
    STREAM<stream_word> offload_out(stream_id("offload_out", rank).c_str());

    AuroraEmuCore aurora_core_0("127.0.0.1", 20000, aurora_id(rank, 0), aurora_id((rank + size - 1) % size, 1), ring_1_out, ring_0_in);
    AuroraEmuCore aurora_core_1("127.0.0.1", 20000, aurora_id(rank, 1), aurora_id((rank + 1) % size, 0), ring_0_out, ring_1_in);

    std::thread offload_kernel(offload, rank, size, std::ref(ring_0_in), std::ref(ring_0_out), std::ref(ring_1_in), std::ref(ring_1_out), std::ref(offload_in), std::ref(offload_out));

    // count is number of channel widths (512bits/64bytes)
    test(Collective::Bcast, Datatype::Double, 64, 2, rank, size, offload_in, offload_out);

    // no way to terminate thread right now, exit by hand here
    offload_kernel.join();

    MPI_Finalize();
}