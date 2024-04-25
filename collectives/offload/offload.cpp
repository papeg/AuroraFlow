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

#include "../collectives.hpp"

#include "common.hpp"

#ifndef SYNTHESIS
#include "offload.hpp"
#include <cstdio>
#endif

class Ring
{
public:
    Ring(int rank, int size) : rank(rank), size(size) {}
    int rank, size;

    void get_bcast_dest(int &east, int &west)
    {
        
    }

    int rank_west()
    {
        return ((rank + size - 1) % size);
    }

    int rank_east()
    {
        return ((rank + 1) % size);
    }

    bool go_west(uint32_t dest)
    {
        uint32_t distance_west = 1;
        uint32_t distance_east = 0;
        return distance_west < distance_east;
    }
};

extern "C"
{
    void offload(int rank, int size,
        STREAM<stream_word> &ring_east_rx, STREAM<stream_word> &ring_east_tx,
        STREAM<stream_word> &ring_west_rx, STREAM<stream_word> &ring_west_tx,
        STREAM<stream_word> &offload_in, STREAM<stream_word> &offload_out)
    {
        Ring ring(rank, size);
        while (true)
        {
            stream_word_union_t header;
            if (ring_east_rx.read_nb(header.word_data))
            {
                switch (header.header.collective)
                {
                    case Collective::Barrier: 
                    case Collective::P2P:
                    case Collective::Bcast:
                        stream_array(header, ring_east_rx, offload_out); 
                        break;
                }
            }
            if (ring_west_rx.read_nb(header.word_data))
            {
                switch (header.header.collective)
                {
                    case Collective::Barrier: 
                    case Collective::P2P:
                    case Collective::Bcast:
                        stream_array(header, ring_west_rx, offload_out); 
                        break;
                }
            }
            if (offload_in.read_nb(header.word_data))
            {
                switch (header.header.collective)
                {
                    case Collective::Barrier: 
                        ring_east_tx.write(header.word_data);
                        break;
                    case Collective::P2P:
                        //TODO: calculate distance in both rings
                        stream_array(header, offload_in, ring_east_tx);
                        break;
                    case Collective::Bcast:
                        header.header.dest = ring.rank_west();
                        stream_array(header, offload_in, ring_east_tx);
                        break;
                }                
            }
        }
    }
}