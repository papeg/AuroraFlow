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
#pragma once

#include <ap_int.h>
#include <ap_axi_sdata.h>

#include <type_traits>
#include <cstdint>

#ifndef SYNTHESIS
#include "hlslib/xilinx/Stream.h"
#define ap_wait() 
#define STREAM hlslib::Stream
#else
#include "hls_stream.h"
#include "etc/autopilot_ssdm_op.h"
#define STREAM hls::stream
#endif

#define DATA_WIDTH 512
#define DEST_WIDTH 0

typedef ap_axiu<DATA_WIDTH, 0, 0, DEST_WIDTH> stream_word;
typedef ap_axiu<32, 0, 0, 0> command_word;

enum Collective {Barrier, P2P, Bcast, Reduce, Scatter, Gather};

enum Datatype {Float, Double, Int, UnsignedInt, Long, UnsignedLong};

enum ReduceOp {Sum, Max, Min, Prod};

typedef struct collectives_header {
    uint32_t collective;
    uint32_t op;
    uint32_t datatype;
    uint32_t count;
    uint32_t root;
    uint32_t dest;
    uint32_t tag; // unused
    uint32_t unused[9];
} collectives_header_t;

template <typename T>
collectives_header_t create_header(Collective collective, uint32_t count, uint32_t root, uint32_t dest = 0, uint32_t op = 0, uint32_t tag = 0)
{
    collectives_header_t header;
    header.collective = collective;
    header.count = count;
    header.root = root;
    header.op = op;
    header.dest = dest;
    header.tag = tag;
    if constexpr (std::is_same<T, double>::value)
    {
        header.datatype = Datatype::Double;
    }
    else if constexpr (std::is_same<T, float>::value)
    {
        header.datatype = Datatype::Float;
    }
    else if constexpr (std::is_same<T, int64_t>::value)
    {
        header.datatype = Datatype::Long;
    }
    else if constexpr (std::is_same<T, uint64_t>::value)
    {
        header.datatype = Datatype::UnsignedLong;
    }
    else if constexpr (std::is_same<T, int32_t>::value)
    {
        header.datatype = Datatype::Int;
    }
    else if constexpr (std::is_same<T, uint32_t>::value)
    {
        header.datatype = Datatype::UnsignedInt;
    }
    return header;
}

typedef union stream_word_union {
    stream_word_union(){};
    ~stream_word_union(){};
    float float_data[16];
    double double_data[8];
    int32_t int32_data[16];
    uint32_t uint32_data[16];
    int64_t int64_data[8];
    uint64_t uint64_data[8];
    stream_word word_data;
    collectives_header_t header;
} stream_word_union_t;

template <typename T>
inline uint32_t get_width()
{
    if constexpr (std::is_same<T, double>::value || std::is_same<T, int64_t>::value || std::is_same<T, uint64_t>::value)
    {
        return 8;
    }
    else if constexpr (std::is_same<T, float>::value || std::is_same<T, int32_t>::value || std::is_same<T, uint32_t>::value)
    {
        return 16;
    }
}

template <typename T>
inline void pack_word(stream_word_union_t &word, T *values)
{
    for (uint32_t i = 0; i < get_width<T>(); i++)
    {
#pragma HLS unroll
        if constexpr (std::is_same<T, double>::value)
        {
            word.double_data[i] = values[i];
        }
        else if constexpr (std::is_same<T, float>::value)
        {
            word.float_data[i] = values[i]; 
        }
        else if constexpr (std::is_same<T, int64_t>::value)
        {
            word.int64_data[i] = values[i];
        }
        else if constexpr (std::is_same<T, uint64_t>::value)
        {
            word.uint64_data[i] = values[i];
        }
        else if constexpr (std::is_same<T, int32_t>::value)
        {
            word.int32_data[i] = values[i];
        }
        else if constexpr (std::is_same<T, uint32_t>::value)
        {
            word.uint32_data[i] = values[i];
        }
    }
}

template <typename T>
inline void unpack_word(stream_word_union_t &word, T *values)
{
    for (uint32_t i = 0; i < get_width<T>(); i++)
    {
#pragma HLS unroll
        if constexpr (std::is_same<T, double>::value)
        {
            values[i] = word.double_data[i];
        }
        else if constexpr (std::is_same<T, float>::value)
        {
            values[i] = word.float_data[i]; 
        }
        else if constexpr (std::is_same<T, int64_t>::value)
        {
            values[i] = word.int64_data[i];
        }
        else if constexpr (std::is_same<T, uint64_t>::value)
        {
            values[i] = word.uint64_data[i];
        }
        else if constexpr (std::is_same<T, int32_t>::value)
        {
            values[i] = word.int32_data[i];
        }
        else if constexpr (std::is_same<T, uint32_t>::value)
        {
            values[i] = word.uint32_data[i];
        }
    }
}

template <typename T>
inline void write_array(STREAM<stream_word> &out, T *values, uint32_t count)
{
    uint32_t width = get_width<T>();
write_array_loop:
    for (uint32_t i = 0; i < count; i += 2)
    {
#pragma HLS protocol fixed
#pragma HLS pipeline II=2
// not enough memory ports for II=1
        stream_word_union_t data_0;
        pack_word(data_0, values + (i * width));
        out.write(data_0.word_data);
// handle case when (count % 2 == 1)
        if (i < count)
        {
            stream_word_union_t data_1;
            pack_word(data_1, values + ((i + 1) * width));
            out.write(data_1.word_data);
        }
    }
}

template <typename T>
inline void read_array(STREAM<stream_word> &in, T *values, uint32_t count)
{
    stream_word_union_t data;
    uint32_t width = get_width<T>();
read_array_loop:
    for (uint32_t i = 0; i < count; i++)
    {
#pragma HLS pipeline II=1
        data.word_data = in.read();
        unpack_word(data, values + (i * width));
    }
}

class ARC
{
public:
    ARC(uint32_t rank, uint32_t size, STREAM<stream_word> &offload_in, STREAM<stream_word> &offload_out) :
        rank(rank), size(size), offload_in(offload_in), offload_out(offload_out) {}
    
    uint32_t rank, size;
    STREAM<stream_word> &offload_in, &offload_out;

    template<typename T>
    void p2p(T *values, uint32_t count, uint32_t root, uint32_t dest)
    {
        stream_word_union_t header;
        if (rank == root)
        {
            header.header = create_header<T>(Collective::P2P, count, root, dest);
            offload_in.write(header.word_data);
            write_array<T>(offload_in, values, count);
        }
        else if (rank == dest)
        {
            header.word_data = offload_out.read();    
            read_array<T>(offload_out, values, count);
        }
    }

    void p2p(double *values, uint32_t count, uint32_t root, uint32_t dest)
    {
        p2p<double>(values, count, root, dest);
    }

    void p2p(float *values, uint32_t count, uint32_t root, uint32_t dest)
    {
        p2p<float>(values, count, root, dest);
    }

    void p2p(int64_t *values, uint32_t count, uint32_t root, uint32_t dest)
    {
        p2p<int64_t>(values, count, root, dest);
    }

    void p2p(uint64_t *values, uint32_t count, uint32_t root, uint32_t dest)
    {
        p2p<uint64_t>(values, count, root, dest);
    }

    void p2p(int32_t *values, uint32_t count, uint32_t root, uint32_t dest)
    {
        p2p<int32_t>(values, count, root, dest);
    }

    void p2p(uint32_t *values, uint32_t count, uint32_t root, uint32_t dest)
    {
        p2p<uint32_t>(values, count, root, dest);
    }

    template <typename T>
    void bcast(T *values, uint32_t count, uint32_t root)
    {
        stream_word_union_t header;
        if (rank == root)
        {
            header.header = create_header<T>(Collective::Bcast, count, root);
            offload_in.write(header.word_data); 
            write_array<T>(offload_in, values, count);
        }
        else
        {
            header.word_data = offload_out.read();    
            read_array<T>(offload_out, values, count);
        }
    }

    void bcast(double *values, uint32_t count, uint32_t root)
    {
        bcast<double>(values, count, root);
    }

    void bcast(float *values, uint32_t count, uint32_t root)
    {
        bcast<float>(values, count, root);
    }

    void bcast(int64_t *values, uint32_t count, uint32_t root)
    {
        bcast<int64_t>(values, count, root);
    }

    void bcast(uint64_t *values, uint32_t count, uint32_t root)
    {
        bcast<uint64_t>(values, count, root);
    }

    void bcast(int32_t *values, uint32_t count, uint32_t root)
    {
        bcast<int32_t>(values, count, root);
    }
    
    void bcast(uint32_t *values, uint32_t count, uint32_t root)
    {
        bcast<uint32_t>(values, count, root);
    }
};