#include <mpi.h>
#include <thread>
#include <memory>

#include "../host/Aurora.hpp"

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


class AuroraRing
{
public:
    AuroraRing(xrt::device &device, xrt::uuid &xclbin_uuid)
        : core_0(Aurora(0, device, xclbin_uuid)), core_1(Aurora(1, device, xclbin_uuid)) {}

    int check_core_status_global(size_t timeout_ms, int world_rank, int world_size)
    {
        int local_core_status[2];

        // barrier so timeout is working for all configurations
        MPI_Barrier(MPI_COMM_WORLD);
        local_core_status[0] = core_0.check_core_status(3000);
        local_core_status[1] = core_1.check_core_status(3000);

        int core_status[2 * world_size];
        MPI_Gather(local_core_status, 2, MPI_INT, core_status, 2, MPI_INT, 0, MPI_COMM_WORLD);

        int errors = 0;
        if (world_rank == 0) {
            for (int i = 0; i < 2 * world_size; i++) {
                if (core_status[i] > 0) {
                    std::cout << "problem with core " << i % 2 << " on rank " << i / 2 << std::endl;
                    errors += 1;
                }
            }
        }
        MPI_Bcast(&errors, 1, MPI_INT, 0, MPI_COMM_WORLD);
        return errors;
    }
    Aurora core_0, core_1;
};

class TestKernel
{
public:
    TestKernel(int rank, int size) : rank(rank), size(size) {}

    virtual void run_test(Collective collective, Datatype datatype, uint32_t count, uint32_t iterations) = 0;

    int rank, size;
};

class HardwareTestKernel: public TestKernel
{
public:
    HardwareTestKernel(int rank, int size, xrt::device &device, xrt::uuid &xclbin_uuid) :
        TestKernel(rank, size)
    {
        offload_kernel = xrt::kernel(device, xclbin_uuid, "offload");
        offload_run = xrt::run(offload_kernel);

        offload_run.set_arg(0, rank);
        offload_run.set_arg(1, size);

        offload_run.start();

        test_kernel = xrt::kernel(device, xclbin_uuid, "test");
        test_run = xrt::run(test_kernel);

        test_run.set_arg(4, rank);
        test_run.set_arg(5, size);
    }

    void run_test(Collective collective, Datatype datatype, uint32_t count, uint32_t iterations)
    {
        test_run.set_arg(0, collective);
        test_run.set_arg(1, datatype);
        test_run.set_arg(2, count);
        test_run.set_arg(3, iterations);

        test_run.start();

        test_run.wait();
    }

    xrt::kernel test_kernel, offload_kernel;
    xrt::run test_run, offload_run;
};

class SoftwareTestKernel: public TestKernel
{
public:
    SoftwareTestKernel(int rank, int size) :
        TestKernel(rank, size),
        ring_0_in(stream_id("ring_0_in", rank).c_str()),
        ring_0_out(stream_id("ring_0_out", rank).c_str()),
        ring_1_in(stream_id("ring_1_in", rank).c_str()),
        ring_1_out(stream_id("ring_1_out", rank).c_str()),
        offload_in(stream_id("offload_in", rank).c_str()),
        offload_out(stream_id("offload_out", rank).c_str()),
        aurora_core_0("127.0.0.1", 20000, aurora_id(rank, 0), aurora_id((rank + size - 1) % size, 1), ring_1_out, ring_0_in),
        aurora_core_1("127.0.0.1", 20000, aurora_id(rank, 1), aurora_id((rank + 1) % size, 0), ring_0_out, ring_1_in)
    {
        if (rank == 0)
        {
            aurora_switch.emplace("127.0.0.1", 20000);
        }
    }

    void run_test(Collective collective, Datatype datatype, uint32_t count, uint32_t iterations)
    {
        // count is number of channel widths (512bits/64bytes)
        test(collective, datatype, count, iterations, rank, size, offload_in, offload_out);
    }

    std::optional<AuroraEmuSwitch> aurora_switch;
    STREAM<stream_word> ring_0_in;
    STREAM<stream_word> ring_0_out;
    STREAM<stream_word> ring_1_in;
    STREAM<stream_word> ring_1_out;
    STREAM<stream_word> offload_in;
    STREAM<stream_word> offload_out;

    AuroraEmuCore aurora_core_0;
    AuroraEmuCore aurora_core_1;
};

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD , &rank);
    MPI_Comm_size(MPI_COMM_WORLD , &size);

    bool emulation = (std::getenv("XCL_EMULATION_MODE") != nullptr);

    if (emulation)
    {
        SoftwareTestKernel test_kernel(rank, size);

        std::thread offload_kernel(offload, rank, size, std::ref(test_kernel.ring_0_in), std::ref(test_kernel.ring_0_out), std::ref(test_kernel.ring_1_in), std::ref(test_kernel.ring_1_out), std::ref(test_kernel.offload_in), std::ref(test_kernel.offload_out));

        for (Collective collective: {Collective::Bcast})
        {
            for (Datatype datatype: {Datatype::Double})
            {
                test_kernel.run_test(collective, datatype, 64, 2);
            }
        }
        // no way to terminate thread right now, exit by hand here
        offload_kernel.join();
    }
    else
    {
        xrt::device device(rank % 3);
        xrt::uuid xclbin_uuid("collectives_test_hw.xclbin");
        AuroraRing aurora_ring(device, xclbin_uuid);
        if (!aurora_ring.check_core_status_global(3000, rank, size))
        {
            MPI_Abort(MPI_COMM_WORLD, rank);
        }
        HardwareTestKernel test_kernel(rank, size, device, xclbin_uuid);

        for (Collective collective: {Collective::Bcast})
        {
            for (Datatype datatype: {Datatype::Double})
            {
                test_kernel.run_test(collective, datatype, 64, 2);
            }
        }
        test_kernel.offload_run.wait();

    }
    MPI_Finalize();
}