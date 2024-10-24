#include <mpi.h>
#include <thread>
#include <chrono>
#include <memory>
#include <fstream>
#include <sstream>
#include <unistd.h>

#include "../../host/Aurora.hpp"

//#include "../collectives.hpp"

class AuroraRing
{
public:
    AuroraRing(xrt::device &device, xrt::uuid &xclbin_uuid)
        : core_0(Aurora(0, device, xclbin_uuid)), core_1(Aurora(1, device, xclbin_uuid)) {}

    AuroraRing() {}

    int check_core_status_global(size_t timeout_ms, int world_rank, int world_size)
    {
        int local_core_status[2];

        // barrier so timeout is working for all configurations
        MPI_Barrier(MPI_COMM_WORLD);
        local_core_status[0] = core_0.core_status_ok(3000);
        local_core_status[1] = core_1.core_status_ok(3000);

        int core_status[2 * world_size];
        MPI_Gather(local_core_status, 2, MPI_INT, core_status, 2, MPI_INT, 0, MPI_COMM_WORLD);

        int errors = 0;
        if (world_rank == 0) {
            for (int i = 0; i < 2 * world_size; i++) {
                if (core_status[i] == 0) {
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

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD , &rank);
    MPI_Comm_size(MPI_COMM_WORLD , &size);

    std::string xcl_emulation_mode = "hw";
    if (std::getenv("XCL_EMULATION_MODE") != nullptr)
    {
        xcl_emulation_mode = std::string(std::getenv("XCL_EMULATION_MODE"));
    }

    if ((xcl_emulation_mode == "hw_emu") && (size != 1))
    {
        std::cout << "hw_emu only works with one rank" << std::endl;
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }

    if (xcl_emulation_mode == "sw_emu")
    {
        std::cout << "not supported (yet)" << std::endl;
    }
    else
    {
        char* hostname;
        hostname = new char[100];
        gethostname(hostname, 100);

        //uint32_t device_id = xcl_emulation_mode == "hw" ? 2 : 0;
        std::string device_string = "0000:01:00.1";
        xrt::device device(0);
        std::cout << "programming device " << 0 << " on rank " << rank << " and host " << hostname << std::endl;

        std::string xclbin_path = "p2p_simplex_u32_hw.xclbin";
        xrt::uuid xclbin_uuid = device.load_xclbin(xclbin_path);

        AuroraRing aurora_ring;
        aurora_ring = AuroraRing(device, xclbin_uuid);
        if (aurora_ring.check_core_status_global(3000, rank, size))
        {
            MPI_Abort(MPI_COMM_WORLD, rank);
        }

        MPI_Barrier(MPI_COMM_WORLD);

        if (rank == 0) {
            std::cout << "Aurora Ring connected" << std::endl;
        }

        std::cout << "waiting for letsgo file" << std::endl;
        while (rename("letsgo", "started") != 0) {}

        uint32_t count = 64;
        uint32_t ref_data[count];
        srand(time(NULL));
        for (uint32_t i = 0; i < count; i++) {
            ref_data[i] = rand();
        }

        xrt::kernel p2p_simplex_u32 = xrt::kernel(device, xclbin_uuid, "p2p_simplex_u32");
        xrt::run p2p_simplex_u32_run = xrt::run(p2p_simplex_u32);

        std::cout << "creating buffer" << std::endl;

        xrt::bo input_buffer = xrt::bo(device, count * sizeof(uint32_t), xrt::bo::flags::normal, p2p_simplex_u32.group_id(2));
        std::cout << "input buffer created" << std::endl;
        input_buffer.write(ref_data);
        input_buffer.sync(XCL_BO_SYNC_BO_TO_DEVICE);

        xrt::bo output_buffer = xrt::bo(device, count * sizeof(uint32_t), xrt::bo::flags::normal, p2p_simplex_u32.group_id(3));

        std::cout << "created buffer" << std::endl;

        p2p_simplex_u32_run.set_arg(0, rank);
        p2p_simplex_u32_run.set_arg(1, count);
        p2p_simplex_u32_run.set_arg(2, input_buffer);
        p2p_simplex_u32_run.set_arg(3, output_buffer);

        p2p_simplex_u32_run.start();

        if (p2p_simplex_u32_run.wait(std::chrono::milliseconds(10000)) == ERT_CMD_STATE_TIMEOUT) {
            std::cout << "Timeout on rank " << rank << std::endl;
        } else {
            uint32_t result_data[count];
            output_buffer.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
            output_buffer.read(result_data);
            if (rank == 0) {
                for (uint32_t i = 0; i < count; i++) {
                    if (ref_data[i] != result_data[i]) {
                        std::cout << i << ": " << ref_data[i] << " != " << result_data[i] << std::endl;
                    }
                }

            }
        }

    }
    MPI_Finalize();
}
