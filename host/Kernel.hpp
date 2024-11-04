class IssueKernel
{
public:
    IssueKernel(uint32_t instance, xrt::device &device, xrt::uuid &xclbin_uuid, Configuration &config) : instance(instance), config(config)
    {
        char name[100];
        snprintf(name, 100, "issue:{issue_%u}", instance);
        kernel = xrt::kernel(device, xclbin_uuid, name);


        data_bo = xrt::bo(device, config.max_num_bytes, xrt::bo::flags::normal, kernel.group_id(1));

        data.resize(config.max_num_bytes);

        char *slurm_job_id = std::getenv("SLURM_JOB_ID");
        unsigned int seed;
        if (slurm_job_id == NULL)
            seed = time(NULL);
        else {
            seed = (unsigned int)std::stoi(slurm_job_id);
        }
        srand(seed);
        for (uint32_t i = 0; i < config.max_num_bytes; i++) {
            data[i] = (config.randomize_data ? rand() : i) % 256;
        }
        data_bo.write(data.data());
        data_bo.sync(XCL_BO_SYNC_BO_TO_DEVICE);
  }

    void prepare_repetition(uint32_t repetition)
    {
        run = xrt::run(kernel);

        run.set_arg(1, data_bo);
        run.set_arg(2, config.message_sizes[repetition]);
        run.set_arg(3, config.frame_size);
        run.set_arg(4, config.iterations_per_message[repetition]);
        run.set_arg(5, config.use_ack);
    }

    void start()
    {
        run.start();
    }

    bool timeout()
    {
        return run.wait(std::chrono::milliseconds(config.timeout_ms)) == ERT_CMD_STATE_TIMEOUT;
    }

    std::vector<char> data;
private:
    xrt::bo data_bo;
    xrt::kernel kernel;
    xrt::run run;
    uint32_t instance;
    Configuration &config;
};

class DumpKernel
{
public:

    DumpKernel(uint32_t instance, xrt::device &device, xrt::uuid &xclbin_uuid, Configuration &config) : instance(instance), config(config)
    {
        char name[100];
        snprintf(name, 100, "dump:{dump_%u}", instance);
        kernel = xrt::kernel(device, xclbin_uuid, name);


        data_bo = xrt::bo(device, config.max_num_bytes, xrt::bo::flags::normal, kernel.group_id(1));

        data.resize(config.max_num_bytes);

    }

    void prepare_repetition(uint32_t repetition)
    {
        run = xrt::run(kernel);

        run.set_arg(1, data_bo);
        run.set_arg(2, config.message_sizes[repetition]);
        run.set_arg(3, config.iterations_per_message[repetition]);
        run.set_arg(4, config.use_ack);
    }

    void start()
    {
        run.start();
    }

    bool timeout()
    {
        return run.wait(std::chrono::milliseconds(config.timeout_ms)) == ERT_CMD_STATE_TIMEOUT;
    }

    void write_back()
    {
        data_bo.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        data_bo.read(data.data());
    }

    uint32_t compare_data(char *ref, uint32_t repetition)
    {
        uint32_t err_num = 0;
        for (uint32_t i = 0; i < config.message_sizes[repetition]; i++) {
            if (data[i] != ref[i]) {
                if (err_num < 16) {
                    printf("dump[%d] = %02x, issue[%d] = %02x\n", i, (uint8_t)data[i], i, (uint8_t)ref[i]);
                }
                err_num++;
            }
        }
        if (err_num) {
            std::cout << "Data verification FAIL" << std::endl;
            std::cout << "for Dump Kernel " << instance << std::endl;
            std::cout << "in repetition " << repetition << std::endl;
            std::cout << "Total mismatched bytes: " << err_num << std::endl;
            std::cout << "Ratio: " << (double)err_num/(double) config.message_sizes[repetition] << std::endl;
        }
        return err_num;
    }

    std::vector<char> data;

private:
    xrt::bo data_bo;
    xrt::kernel kernel;
    xrt::run run;
    uint32_t instance;
    Configuration &config;
};

