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
#include "rx.hpp"
#include <cstdio>
#endif

extern "C"
{
    void tx(int rank, int size,
        STREAM<stream_word> &ring_in,
        STREAM<stream_word> &ring_out,
        STREAM<stream_word> &offload_in)
    {
        stream_word_union_t header;
        while (true)
        {
            if (ring_in.read_nb(header.word_data))
            {
                stream_array(header, ring_in, ring_out);
            }
            if (offload_in.read_nb(header.word_data))
            {
                stream_array(header, offload_in, ring_out);
            }
        }
    }
}
 