/*
 * Copyright 2024 Gerrit Pape (papeg@mail.upb.de)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "collectives.hpp"

#ifndef SYNTHESIS
#include "test.hpp"
#include <cstdio>
#endif

uint32_t count_max = 2048;

template <typename T>
void fill_data(T *data)
{
    T value = 1;
fill_data_loop:
    for (uint32_t i = 0; i < (count_max * get_width<T>()); i++)
    {
#pragma HLS pipeline off
        data[i] = value;
        value *= 2;
    }
}


template <typename T>
void zero_data(T *data)
{
zero_data_loop:
    for (uint32_t i = 0; i < (count_max * get_width<T>()); i++)
    {
        data[i] = 0;
    }
}

template <typename T>
uint32_t compare_data(bool check_errors, uint32_t collective, T *data, T *ref, uint32_t count, int rank, uint32_t dest = 0)
{
    uint32_t errors = 0;
    if (check_errors)
    {
        if (((collective == Collective::P2P) && (rank == dest))
            || (collective == Collective::Bcast))
        {
compare_data_loop:
            for (uint32_t i = 0; i < (count * get_width<T>()); i++)
            {
                if (data[i] != ref[i])
                {
                    errors += 1;
                }
            }
        #ifndef SYNTHESIS
            printf("%u errors\n", errors);
        #endif
        }
    }
    return errors;
}

template <typename T>
void prepare_data(int rank, T *data, T *ref)
{
    fill_data<T>(ref);
    if (rank == 0)
    {
        fill_data<T>(data);
    }
    else
    {
        zero_data<T>(data);
    }
}

extern "C"
{
    void test(uint32_t collective, uint32_t datatype, uint32_t count, uint32_t iterations, uint32_t dest, int rank, int size, bool check_errors, uint32_t *errors_out, STREAM<stream_word> &offload_in, STREAM<stream_word> &offload_out)
    {
        // test data
        float float_data[count_max * 16];
        float float_ref[count_max * 16];
        double double_data[count_max * 8];
        double double_ref[count_max * 8];

        int32_t int32_data[count_max * 16];
        int32_t int32_ref[count_max * 16];
        uint32_t uint32_data[count_max * 16];
        uint32_t uint32_ref[count_max * 16];

        int64_t int64_data[count_max * 8];
        int64_t int64_ref[count_max * 8];
        uint64_t uint64_data[count_max * 8];
        uint64_t uint64_ref[count_max * 8];

        uint32_t errors = 0;
        ARC arc(rank, size, offload_in, offload_out);
        for (uint32_t i = 0; i < iterations; i++)
        {
            switch (datatype)
            {
                case Datatype::Float:
                    break;
                case Datatype::Double:
                    prepare_data<double>(rank, double_data, double_ref);
                    switch (collective)
                    {
                        case Collective::P2P:
                            arc.p2p(double_data, count, 0, dest);
                            break;
                        case Collective::Bcast:
                            arc.bcast(double_data, count, 0);
                            break;
                    }
                    errors += compare_data<double>(check_errors, collective, double_data, double_ref, count, rank, dest);
                    break;
                case Datatype::Int:
                    prepare_data<int32_t>(rank, int32_data, int32_ref);
                    switch (collective)
                    {
                        case Collective::P2P:
                            arc.p2p(int32_data, count, 0, dest);
                            break;
                        case Collective::Bcast:
                            arc.bcast(int32_data, count, 0);
                            break;
                    }
                    errors += compare_data<int32_t>(check_errors, collective, int32_data, int32_ref, count, rank, dest);
                    break;
                case Datatype::UnsignedInt:
                case Datatype::Long:
                case Datatype::UnsignedLong:
                    break;
            }
        }
        *errors_out = errors;
    }
}
