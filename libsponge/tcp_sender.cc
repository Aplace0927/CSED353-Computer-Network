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

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    // Already FINed or Not yet SYNed?
    //  -> Do nothing (FINed) / Send with SYN flag (Not SYNed)

    // Window size = max(1, (current Window size))

    // WHILE Window is able to push more:
    //  ... Make a packet with data from stream (size up to MAX_PAYLOAD_SIZE), and send it.
    // IF Nothing to be sent?
    //  ... Terminate.
    // IF Packet is last one from stream?
    //  ... Raise FIN flag.
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
}

unsigned int TCPSender::consecutive_retransmissions() const { return _timer.consec_retransmit; }

void TCPSender::send_empty_segment() {
    TCPSegment empty_tcpseg;
    empty_tcpseg.header().seqno = wrap(_next_seqno, _isn);  // Empty packet with correct sequence number
    _segments_out.push(empty_tcpseg);
}
