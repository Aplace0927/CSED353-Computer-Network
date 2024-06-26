#include "tcp_receiver.hh"

#include <iostream>
// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    _syn |= seg.header().syn;
    if (not _syn) {
        return;  // Never met `SYN` before.
    }
    if (seg.header().syn) {
        _isn = seg.header().seqno;  // Initialize ISN offset.
    }

    _chkpoint += seg.length_in_sequence_space();  // Accumulate to checkpoint as a reference of unwrapping

    uint64_t stream_idx =
        /*                   SEQNO                Skip `SYN` byte on first packet           ISN   CHECKPOINT*/
        unwrap(WrappingInt32(seg.header().seqno + static_cast<uint32_t>(seg.header().syn)), _isn, _chkpoint) - 1;

    _reassembler.push_substring(seg.payload().copy(), stream_idx, seg.header().fin);
    return;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (not _syn) {
        return nullopt; /* State: LISTEN */
    }
    /*          SYN    DATA LENGTH                    FIN?                                                   */
    return wrap(1ULL + stream_out().bytes_written() + static_cast<uint64_t>(stream_out().input_ended()), _isn);
}

size_t TCPReceiver::window_size() const { return stream_out().remaining_capacity(); }
