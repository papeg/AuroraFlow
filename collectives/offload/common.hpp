
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

inline void fork_array_dest(stream_word_union_t &header, STREAM<stream_word> &in, STREAM<stream_word> &out_0, uint32_t dest_0, STREAM<stream_word> &out_1, uint32_t dest_1)
{
io_section:
{
#pragma HLS pipeline fixed
    header.header.dest = dest_0;
    out_0.write(header.word_data);
    ap_wait();
    header.header.dest = dest_1;
    out_1.write(header.word_data);
}
fork_array_dest_loop:
    for (uint32_t i = 0; i < header.header.count; i++)
    {
#pragma HLS pipeline II=1
        stream_word word = in.read();
        out_0.write(word);
        out_1.write(word);
    }
}