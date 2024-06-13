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
#include "hls_task.h"

#include "offload.hpp"
#include "rx.hpp"
#include "tx.hpp"

extern "C"
{
    void arc(int rank, int size,
        STREAM<stream_word> &offload_in, STREAM<stream_word> &offload_out,
        STREAM<stream_word> &rx_east_ring_in, STREAM<stream_word> &rx_west_ring_in,
        STREAM<stream_word> &tx_east_ring_out, STREAM<stream_word> &tx_west_ring_out)
    {
        hls_thread_local STREAM<stream_word> east_ring;
        hls_thread_local STREAM<stream_word> west_ring;

        hls_thread_local STREAM<stream_word> rx_east_offload_out;
        hls_thread_local STREAM<stream_word> rx_west_offload_out;

        hls_thread_local STREAM<stream_word> tx_east_offload_in;
        hls_thread_local STREAM<stream_word> tx_west_offload_in;

        hls_thread_local hls::task offload_task(offload, rank, size, rx_east_offload_out, tx_east_offload_in, rx_west_offload_out, tx_west_offload_in, offload_in, offload_out);

        hls_thread_local hls::task rx_east_task(rx, rank, size, rx_east_ring_in, east_ring, rx_east_offload_out);
        hls_thread_local hls::task rx_west_task(rx, rank, size, rx_west_ring_in, west_ring, rx_west_offload_out);

        hls_thread_local hls::task tx_east_task(tx, rank, size, east_ring, tx_east_ring_out, tx_east_offload_in);
        hls_thread_local hls::task tx_west_task(tx, rank, size, west_ring, tx_west_ring_out, tx_west_offload_in);
    }
}