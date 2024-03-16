#include "tcp_receiver.hh"

#include <iostream>
// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    _syn |= seg.header().syn;
    if (not _syn) {
        return;
    }
    if (seg.header().syn) {
        _isn = seg.header().seqno;
    }

    _chkpoint += seg.length_in_sequence_space();

    uint64_t stream_idx =
        unwrap(WrappingInt32(seg.header().seqno + static_cast<uint32_t>(seg.header().syn)), _isn, _chkpoint) - 1;

    _reassembler.push_substring(seg.payload().copy(), stream_idx, seg.header().fin);
    return;
}

optional<WrappingInt32> TCPReceiver::ackno() const { return {}; }

size_t TCPReceiver::window_size() const { return {}; }
