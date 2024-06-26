#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

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

    if (seg.header().rst) {
        close(false);  // Unclean shutdown initiated from server-side, set as error
        return;
    }

    _receiver.segment_received(seg);

    // 3-Way Handshaking, 2nd stage
    if (TCPState::state_summary(_sender) == TCPSenderStateSummary::CLOSED and
        TCPState::state_summary(_receiver) == TCPReceiverStateSummary::SYN_RECV) {
        connect();
        return;
    }

    // ACKing to TCP Connection: send new data, with updating connection environment (ex: Window size).
    if (seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
    }

    // IF No data is available, then send empty segment as new data. (implicitly meaning data end)
    if (seg.length_in_sequence_space() > 0 and _sender.segments_out().empty()) {
        _sender.send_empty_segment();
    }

    // Keep-Alive
    if (_receiver.ackno().has_value() and (seg.length_in_sequence_space() == 0) and
        seg.header().seqno == _receiver.ackno().value() - 1) {
        _sender.send_empty_segment();
    }

    launch(false);  // Send ACK segment.
    close(true);    // Clean shutdown, FIN segment.
}

bool TCPConnection::active() const { return activeness; }

size_t TCPConnection::write(const string &data) {
    if ((not active()) or (data.length() == 0)) {
        return 0UL;
    }
    /**
     * Write string to sender's stream, and sender will send it.
     */
    size_t written = _sender.stream_in().write(data);
    _sender.fill_window();
    launch(false);
    return written;
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
    _sender.tick(ms_since_last_tick);
    last_seg_recv += ms_since_last_tick;
    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        _sender.fill_window();
        if (_sender.segments_out().empty()) {
            _sender.send_empty_segment();
        }
        launch(true);  // Send RST segment.
        close(false);  // Unclean shutdown.
        return;
    }
    launch(false);  // Send ACK segment.
    close(true);    // Clean shutdown, FIN segment.
    return;
}

void TCPConnection::end_input_stream() { /* End the input stream (Trigger EOF) -> Send FIN */
    _sender.stream_in().end_input();
    _sender.fill_window();
    launch(false);
    return;
}

void TCPConnection::connect() { /* Do the 1st step (SYN) of 3-Way handshake */
    _sender.fill_window();
    launch(false);
    return;
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
        if ((not _sender.stream_in().eof()) and (_receiver.stream_out().input_ended())) {
            _linger_after_streams_finish = false;
            return;
        }
        if (TCPState::state_summary(_sender) == TCPSenderStateSummary::FIN_ACKED and
            TCPState::state_summary(_receiver) == TCPReceiverStateSummary::FIN_RECV) {
            /**
             * Maintain active connection when
             * - Should be lingered AND
             * - (Last segment recieved time) is shorter than (10 * RT_TIMEOUT)
             */
            activeness &=
                (_linger_after_streams_finish) and (time_since_last_segment_received() < 10 * _cfg.rt_timeout);
        }
        return;
    }
}

void TCPConnection::launch(const bool reset) {
    TCPSegment payload;
    while ((not _sender.segments_out().empty()) or reset) {
        payload = _sender.segments_out().front();
        _sender.segments_out().pop();
        if (_receiver.ackno().has_value()) {  // ACKing to the recieved packet
            payload.header().ack = true;
            payload.header().ackno = _receiver.ackno().value();
            payload.header().win =
                min(static_cast<uint16_t>(_receiver.window_size()),
                    numeric_limits<uint16_t>::max());  // No congestion control, send as much as possible
        }
        payload.header().rst = reset;
        _segments_out.push(payload);
        if (reset) {  // RST sends only one packet.
            return;
        }
    }
    return;
}
TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            _sender.fill_window();
            if (_sender.segments_out().empty()) {
                _sender.send_empty_segment();
            }
            launch(true);  // Send RST segment
            close(false);  // Unclean shutdown
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
