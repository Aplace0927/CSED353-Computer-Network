#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return (_next_seqno - _curr_ackno); }

void TCPSender::fill_window() {
    TCPSegment tcpseg;

    // Not SYNed yet (CLOSED) -> Send SYN and raise the SYN flag
    if (_status_closed()) {
        tcpseg.header().syn = true;
        _send_tcp_segment(tcpseg, true);
        return;
    }

    // SYN sent, not ACKed to SYN yet -> Wait for ACK from other side.
    if (_status_syn_sent()) {
        return;
    }

    // Window size = max(1, (current Window size))
    size_t window_sz = max(static_cast<uint16_t>(1), _rwnd_size);

    // WHILE Window is able to push more:
    while (window_sz > bytes_in_flight()) {
        // Current FSM status has chance to send any data.
        if (_status_syn_acked() || _status_syn_acked_reached_eof()) {
            // Make a packet with data from stream (size up to MAX_PAYLOAD_SIZE)
            tcpseg.payload() = Buffer(_stream.read(min(window_sz - bytes_in_flight(), TCPConfig::MAX_PAYLOAD_SIZE)));

            // IF stream end? -> Raise a FIN flag
            tcpseg.header().fin = _status_syn_acked_reached_eof();

            if (tcpseg.length_in_sequence_space() == 0) {
                break;  // Terminate on empty TCP segment.
            }

            // Send the segment, and wait for the response.
            _send_tcp_segment(tcpseg, true);
        }

        // Current FSM status is already finished transmitting.
        if (_status_fin_sent() or _status_fin_acked()) {
            break;
        }
    }
    return;
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // Check validness
    uint64_t recv_ackno = unwrap(ackno, _isn, _curr_ackno);
    if (_next_seqno < recv_ackno) {
        return;
    }

    // Update window size, ackno
    _rwnd_size = window_size;
    _curr_ackno = max(_curr_ackno, recv_ackno);

    // Remove the packet in _segment_out queue which is outdated
    //  ... Pop every packet WHILE _segment_out's front seqno is earlier than ackno.
    while ((not _segments_outstand.empty()) and (unwrap(_segments_outstand.front().header().seqno, _isn, _next_seqno) +
                                                     _segments_outstand.front().length_in_sequence_space() <=
                                                 recv_ackno)) {
        _bytes_in_flight -= _segments_outstand.front().length_in_sequence_space();
        _segments_outstand.pop();
    }
    // Then fill the window & send with new data.
    fill_window();

    // Initialize Retrns. Timeout accumulating state.
    _timer.reset(_initial_retransmission_timeout);

    // IF there are still unsent packet exists?
    if (not _segments_outstand.empty()) {
        _timer.restart();  //  Start Retrns. Timeout timer.
    }

    return;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _timer.elapsed_ticks += ms_since_last_tick;  // Accumulate `ms_since_last_tick` to internal tick count

    // IF there are no packets to be sent?
    if (_segments_outstand.empty()) {
        // Timer stop, return.
        _timer.working = false;
        return;
    }

    // IF ([internal tick count] >= Retrns.Timeout)  (and also there are packets to be sent) ?
    if (_timer.timeover()) {
        _segments_out.push(_segments_outstand.front());
        _timer.consec_retransmit += 1;  // Accumulate consecutive retransmit.
        _timer.rto *= 2;                // Double the RTO
        _timer.restart();               // Restart counter
    }
    return;
}

unsigned int TCPSender::consecutive_retransmissions() const { return _timer.consec_retransmit; }

void TCPSender::send_empty_segment() {
    TCPSegment empty_tcpseg;
    _send_tcp_segment(empty_tcpseg, false);
    return;
}

void TCPSender::_send_tcp_segment(TCPSegment &tcpseg, bool wait_response) {
    tcpseg.header().seqno = wrap(_next_seqno, _isn);
    _next_seqno += tcpseg.length_in_sequence_space();
    _bytes_in_flight += tcpseg.length_in_sequence_space();

    _segments_out.push(tcpseg);
    if (wait_response) {
        _segments_outstand.push(tcpseg);
    }

    _timer.restart();
    return;
}

bool TCPSender::_status_closed() { return next_seqno_absolute() == 0ULL; }
bool TCPSender::_status_syn_sent() {
    return next_seqno_absolute() > 0ULL and next_seqno_absolute() == bytes_in_flight();
}
bool TCPSender::_status_syn_acked() { return next_seqno_absolute() > bytes_in_flight() and not stream_in().eof(); }
bool TCPSender::_status_syn_acked_reached_eof() {
    return stream_in().eof() and next_seqno_absolute() < stream_in().bytes_written() + 2ULL;
}
bool TCPSender::_status_fin_sent() {
    return stream_in().eof() and next_seqno_absolute() == stream_in().bytes_written() + 2ULL and bytes_in_flight() > 0;
}
bool TCPSender::_status_fin_acked() {
    return stream_in().eof() and next_seqno_absolute() == stream_in().bytes_written() + 2ULL and bytes_in_flight() == 0;
}