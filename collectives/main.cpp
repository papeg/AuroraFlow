#include <mpi.h>
#include <thread>
#include <chrono>
#include <memory>

#include "../host/Aurora.hpp"

#include "collectives.hpp"
#include "auroraemu.hpp"

#include "./offload/rx.hpp"
#include "./offload/tx.hpp"
#include "./offload/offload.hpp"
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

    virtual uint32_t run_test(Collective collective, Datatype datatype, uint32_t count, uint32_t iterations, uint32_t dest) = 0;

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

        errors_bo = xrt::bo(device, 1, xrt::bo::flags::normal, test_kernel.group_id(6));

        test_run.set_arg(6, errors_bo);
    }

    void write_metrics(Collective collective, Datatype datatype, uint32_t errors, uint32_t count, uint32_t iterations, double run_time)
    {
        while (rename("./build/results.csv", "./build/results.csv.lock") != 0) {}
        std::ofstream of;
        of.open("./build/results.csv.lock", std::ios_base::app);
        of << rank << "," << size << "," << collective << "," << datatype << "," << errors << "," << count << "," << iterations << "," << run_time << std::endl;
        of.close();
        rename("./build/result.csv.lock", "./build/results.csv");
    }

    uint32_t run_test(Collective collective, Datatype datatype, uint32_t count, uint32_t iterations, uint32_t dest = 0)
    {
        test_run.set_arg(0, collective);
        test_run.set_arg(1, datatype);
        test_run.set_arg(2, count);
        test_run.set_arg(3, iterations);

        double start_time = get_wtime();
        test_run.start();

        test_run.wait();
        double run_time = get_wtime() - start_time;

        uint32_t errors;
        errors_bo.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        errors_bo.read(&errors);

        double run_time_max;
        uint32_t errors_sum;
        MPI_Reduce(&run_time, &run_time_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        MPI_Reduce(&errors, &errors_sum, 1, MPI_UNSIGNED, MPI_SUM, 0, MPI_COMM_WORLD);
        if (rank == 0)
        {
            write_metrics(collective, datatype, errors_sum, count, iterations, run_time_max);
        }
        MPI_Barrier(MPI_COMM_WORLD);
        return errors;
    }

    xrt::kernel test_kernel, offload_kernel;
    xrt::run test_run, offload_run;
    xrt::bo errors_bo;
    std::string metrics;
};

class SoftwareTestKernel: public TestKernel
{
public:
    SoftwareTestKernel(int rank, int size) :
        TestKernel(rank, size),
        rx_east_ring_in(stream_id("rx_east_ring_in", rank).c_str()),
        rx_east_ring_out(stream_id("rx_east_ring_out", rank).c_str()),
        rx_east_offload_out(stream_id("rx_east_offload_out", rank).c_str()),
        rx_west_ring_in(stream_id("rx_west_ring_in", rank).c_str()),
        rx_west_ring_out(stream_id("rx_west_ring_out", rank).c_str()),
        rx_west_offload_out(stream_id("rx_west_offload_out", rank).c_str()),

        tx_east_ring_out(stream_id("tx_east_ring_out", rank).c_str()),
        tx_east_offload_in(stream_id("tx_east_offload_out", rank).c_str()),
        tx_west_ring_out(stream_id("tx_west_ring_out", rank).c_str()),
        tx_west_offload_in(stream_id("tx_west_offload_out", rank).c_str()),

        offload_in(stream_id("offload_in", rank).c_str()),
        offload_out(stream_id("offload_out", rank).c_str()),
        aurora_core_0("127.0.0.1", 20000, aurora_id(rank, 0), aurora_id((rank + size - 1) % size, 1), tx_west_ring_out, rx_east_ring_in),
        aurora_core_1("127.0.0.1", 20000, aurora_id(rank, 1), aurora_id((rank + 1) % size, 0), tx_east_ring_out, rx_west_ring_in)
    {
        if (rank == 0)
        {
            aurora_switch.emplace("127.0.0.1", 20000);
        }
    }

    uint32_t run_test(Collective collective, Datatype datatype, uint32_t count, uint32_t iterations, uint32_t dest = 0)
    {
        // count is number of channel widths (512bits/64bytes)
        uint32_t errors = 0;
        test(collective, datatype, count, iterations, dest, rank, size, true, &errors, offload_in, offload_out);
        uint32_t errors_sum;
        MPI_Reduce(&errors, &errors_sum, 1, MPI_UNSIGNED, MPI_SUM, 0, MPI_COMM_WORLD);
        return errors_sum;
    }

    std::optional<AuroraEmuSwitch> aurora_switch;
    STREAM<stream_word> rx_east_ring_in;
    STREAM<stream_word> rx_east_ring_out;
    STREAM<stream_word> rx_east_offload_out;
    STREAM<stream_word> rx_west_ring_in;
    STREAM<stream_word> rx_west_ring_out;
    STREAM<stream_word> rx_west_offload_out;

    STREAM<stream_word> tx_east_ring_out;
    STREAM<stream_word> tx_east_offload_in;
    STREAM<stream_word> tx_west_ring_out;
    STREAM<stream_word> tx_west_offload_in;

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

        std::thread offload_kernel(offload, rank, size, std::ref(test_kernel.rx_east_offload_out), std::ref(test_kernel.tx_east_offload_in), std::ref(test_kernel.rx_west_offload_out), std::ref(test_kernel.tx_west_offload_in), std::ref(test_kernel.offload_in), std::ref(test_kernel.offload_out));
        std::thread rx_east(rx, rank, size, std::ref(test_kernel.rx_east_ring_in), std::ref(test_kernel.rx_east_ring_out), std::ref(test_kernel.rx_east_offload_out));
        std::thread rx_west(rx, rank, size, std::ref(test_kernel.rx_west_ring_in), std::ref(test_kernel.rx_west_ring_out), std::ref(test_kernel.rx_west_offload_out));
        std::thread tx_east(tx, rank, size, std::ref(test_kernel.rx_east_ring_out), std::ref(test_kernel.tx_east_ring_out), std::ref(test_kernel.tx_east_offload_in));
        std::thread tx_west(tx, rank, size, std::ref(test_kernel.rx_west_ring_out), std::ref(test_kernel.tx_west_ring_out), std::ref(test_kernel.tx_west_offload_in));

        // wait for threads to startup
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1000ms);

        for (Collective collective: {Collective::P2P, Collective::Bcast})
        {
            for (Datatype datatype: {Datatype::Double})
            {
                if (rank == 0)
                {
                    std::cout << "testing collective: " << collective << ", datatype: " << datatype << std::endl;
                }
                if (collective == Collective::P2P)
                {
                    for (int dest = 1; dest < size; dest++)
                    {
                        test_kernel.run_test(collective, datatype, 64, 4, dest);
                    }
                }
                else
                {
                    test_kernel.run_test(collective, datatype, 64, 4);
                }
            }
        }
        if (rank == 0)
        {
            std::cout << "all tests have run" << std::endl;
        }
        // no way to terminate thread right now, exit by hand here
        offload_kernel.join();
    }
    else
    {
        uint32_t total_errors = 0;
        xrt::device device(rank % 3);
        xrt::uuid xclbin_uuid = device.load_xclbin("collectives_test_hw.xclbin");
        AuroraRing aurora_ring(device, xclbin_uuid);
        if (aurora_ring.check_core_status_global(3000, rank, size))
        {
            MPI_Abort(MPI_COMM_WORLD, rank);
        }
        HardwareTestKernel test_kernel(rank, size, device, xclbin_uuid);

        uint32_t iterations = 1;
        uint32_t count_max = 2048;
        for (Collective collective: {Collective::Bcast})
        {
            for (Datatype datatype: {Datatype::Double})
            {
                for (uint32_t count = 1; count <= count_max; count <<= 1)
                {
                    if (rank == 0)
                    {
                        std::cout << "testing collective: " << collective << ", datatype: " << datatype << ", count: " << count << std::endl;
                    }
                    uint32_t errors = test_kernel.run_test(collective, datatype, count, iterations);
                    std::cout << "errors: " << errors << std::endl;
                    total_errors += errors;
                    MPI_Barrier(MPI_COMM_WORLD);
                }
            }
        }
        if (rank == 0) {
            std::cout << "All tests have run, total errors: " << total_errors << std::endl;
        }
    }
    MPI_Finalize();
}
