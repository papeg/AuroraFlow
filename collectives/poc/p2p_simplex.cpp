#include "../collectives.hpp"
#include "../offload/common.hpp"

template <typename T>
void p2p_simplex(uint32_t rank, uint32_t count, T *input, T *output, STREAM<stream_word> &ring_in, STREAM<stream_word> &ring_out)
{
#pragma HLS protocol fixed
    stream_word_union_t header;
    if (rank == 0) {
        header.header = create_header<T>(Collective::P2P, count, 0);
        ring_out.write(header.word_data);
        ap_wait();
        write_array<T>(ring_out, input, count);
        ap_wait();

        header.word_data = ring_in.read(); 
        ap_wait();
        read_array<T>(ring_in, output, count);
    } else {
        header.word_data = ring_in.read();
        ap_wait();
        stream_array(header, ring_in, ring_out);
    }
}

extern "C"
{
    void p2p_simplex_u32(uint32_t rank, uint32_t count, uint32_t *input, uint32_t *output, STREAM<stream_word> &ring_in, STREAM<stream_word> &ring_out)
    {
        p2p_simplex<uint32_t>(rank, count, input, output, ring_in, ring_out);
    }
}
