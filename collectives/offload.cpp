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

inline void stream_array(stream_word_union_t &header, STREAM<stream_word> &in, STREAM<stream_word> &out)
{
    out.write(header.word_data);
stream_array_loop:
    for (uint32_t i = 0; i < header.header.count; i++)
    {
#pragma HLS pipeline II=1
        out.write(in.read());
    }
}

inline void fork_array(stream_word_union_t &header, STREAM<stream_word> &in, STREAM<stream_word> &out_0, STREAM<stream_word> &out_1)
{
    out_0.write(header.word_data);
    out_1.write(header.word_data);
fork_array_loop:
    for (uint32_t i = 0; i < header.header.count; i++)
    {
#pragma HLS pipeline II=1
        stream_word word = in.read();
        out_0.write(word);
        out_1.write(word);
    }
}


extern "C"
{
    void offload(int rank, int size,
        STREAM<stream_word> &ring_0_in, STREAM<stream_word> &ring_0_out,
        STREAM<stream_word> &ring_1_in, STREAM<stream_word> &ring_1_out,
        STREAM<stream_word> &offload_in, STREAM<stream_word> &offload_out)
    {
        while (true)
        {
            stream_word_union_t header;
            if (ring_0_in.read_nb(header.word_data))
            {
                switch (header.header.collective)
                {
                    case Collective::Barrier: 
                        if (header.header.root != rank) {
                            ring_0_out.write(header.word_data);
                        }
                        break;
                    case Collective::P2P:
                        if (header.header.root == rank)
                        {
                            stream_array(header, ring_0_in, offload_out);
                        }
                        else
                        {
                            stream_array(header, ring_0_in, ring_0_out);
                        }
                        break;
                    case Collective::Bcast:
                        if (header.header.root != rank)
                        {
                            fork_array(header, ring_0_in, ring_0_out, offload_out);
                        }
                        break;
                }
            }
            if (offload_in.read_nb(header.word_data))
            {
                switch (header.header.collective)
                {
                    case Collective::Barrier: 
                        ring_0_out.write(header.word_data);
                        break;
                    case Collective::P2P:
                    case Collective::Bcast:
                        stream_array(header, offload_in, ring_0_out);
                        break;
                }                
            }
            // hint for unused ring 1
            if (rank > 999999)
                ring_1_out.write(ring_1_in.read());
        }
    }
}