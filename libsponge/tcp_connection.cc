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

void TCPConnection::segment_received(const TCPSegment &seg) { DUMMY_CODE(seg); }

bool TCPConnection::active() const { return activeness; }

size_t TCPConnection::write(const string &data) {
    DUMMY_CODE(data);
    return {};
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { DUMMY_CODE(ms_since_last_tick); }

void TCPConnection::end_input_stream() {}

void TCPConnection::connect() {}

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
