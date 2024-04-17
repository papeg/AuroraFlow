#include "collectives.hpp"

#ifndef SYNTHESIS
#include <cstdio>
#endif

uint32_t count_max = 2048;

void fill_data(double *data)
{
    double value = 1.0;
fill_data_loop:
    for (uint32_t i = 0; i < (count_max / 8); i++)
    {
#pragma HLS pipeline off
        data[i] = value;
        value *= 2.0;
    }
}

void zero_data(double *data)
{
    double value = 1.0;
zero_data_loop:
    for (uint32_t i = 0; i < (count_max / 8); i++)
    {
        data[i] = 0.0;
    }
}

uint32_t compare_data(double *data, double *ref)
{
    uint32_t errors = 0;
compare_data_loop:
    for (uint32_t i = 0; i < (count_max / 8); i++)
    {
        if (data[i] != ref[i])
        {
            errors += 1;
        }
    }
#ifndef SYNTHESIS
    printf("%u errors\n", errors);
#endif
    return errors;
}

extern "C"
{
    void test(uint32_t collective, uint32_t datatype, uint32_t count, uint32_t iterations, uint32_t rank, uint32_t size, STREAM<stream_word> &offload_in, STREAM<stream_word> &offload_out)
    {
        // test data
        double double_data[count_max / 8];
        double ref_data[count_max / 8];
        fill_data(ref_data);
        if (rank == 0)
        {
            fill_data(double_data);
        }
        else
        {
            zero_data(double_data);
        }

        ARC arc(rank, size, offload_in, offload_out);
        if (collective == Collective::Bcast)
        {
            for (uint32_t i = 0; i < iterations; i++)
            {
#pragma HLS pipeline off
                if (datatype == Datatype::Double)
                {
                    arc.bcast(double_data, count, 0);
                }
            }
            compare_data(double_data, ref_data);
        }
    }
}
