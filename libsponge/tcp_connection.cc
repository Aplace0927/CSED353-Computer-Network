#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return last_seg_recv; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    if (not active()) {
        return;
    }
    last_seg_recv = 0UL;  // Reset tick elapsed from last segment recieved

    /**
     * IF RST Segment?
     *  - Then Close "Ungracefully".
     *
     * IF 3-Way Handshake not finished, in Reciever's view?
     *  - Then (SYN + ACK) segment should be sent.
     *
     * IF Recieved Packet is ACKing to TCP Connection?
     *  - Then send new data, with updating connection environment (ex: Window size).
     *  - IF No data is available, then send empty segment as new data. (implicitly meaning data end)
     *
     * (+ Keep-Alive Implementation)
     */

    if (seg.header().rst) {
        close(false);
        return;
    }

    _receiver.segment_received(seg);

    // 3-Way Handshaking
    if (TCPState(_sender, _receiver, activeness, _linger_after_streams_finish) == TCPState(TCPState::State::SYN_RCVD)) {
        // SYN + ACK Should be sent
    }

    // ACKing to TCP Connection.
    if (seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
        if (seg.length_in_sequence_space() > 0 and _sender.segments_out().empty()) {
            _sender.send_empty_segment();
        }
    }

    // Keep-Alive
    if (_receiver.ackno().has_value() and (seg.length_in_sequence_space() == 0) and
        seg.header().seqno == _receiver.ackno().value() - 1) {
        _sender.send_empty_segment();
    }
}

bool TCPConnection::active() const { return activeness; }

size_t TCPConnection::write(const string &data) {
    if (not active()) {
        return 0UL;
    }
    DUMMY_CODE(data);
    /**
     * Write string to sender's stream, and sender will send it.
     */
    return {};
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    if (not active()) {
        return;
    }
    /**
     * Update tick.
     *
     * IF (# of Consec. retransmissions) > MAX_RETX_ATTEMPTS
     *  - Close connection "ungracefully"
     */
    DUMMY_CODE(ms_since_last_tick);
}

void TCPConnection::end_input_stream() { /* End the input stream (Trigger EOF) -> Send FIN */
}

void TCPConnection::connect() { /* Do the 1st step (SYN) of 3-Way handshake */
}

void TCPConnection::close(const bool graceful) {
    if (not graceful) {
        /* Errorful closing could be achieved by spraying error on in/out stream. */
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        activeness = false;
        return;
    } else {
        /* Turn the 'Lingering' switch when EOF had reached*/
        _linger_after_streams_finish &= (not((_receiver.stream_out().input_ended()) and (_sender.stream_in().eof())));
        if (TCPState::state_summary(_sender) == TCPSenderStateSummary::FIN_ACKED and
            TCPState::state_summary(_receiver) == TCPReceiverStateSummary::FIN_RECV) {
            /**
             * Maintain active connection when
             * - Should be lingered AND
             * - (Last segment recieved time) is shorter than (10 * RT_TIMEOUT)
             */
            activeness &= (_linger_after_streams_finish and time_since_last_segment_received() < 10 * _cfg.rt_timeout);
        }
        return;
    }
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
