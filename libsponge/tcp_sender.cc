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
    // Check validness, outdated ack

    // Update window size, ackno

    // Remove the packet in _segment_out queue which is outdated
    //  ... Pop every packet WHILE _segment_out's front seqno is earlier than ackno.
    // Then fill the window & send with new data.

    // Initialize Retrns. Timeout accumulating state.

    // IF there are still unsent packet exists?
    //  ... Start Retrns. Timeout timer.
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    // IF there are no packets to be sent?
    //      -> Do not accumulate, return.

    // Accumulate `ms_since_last_tick` to internal tick count

    // IF ([internal tick count] >= Retrns.Timeout)  (and also there are packets to be sent) ?
    //      -> Double the Retrns. Timeout and restart accumulating
    //      -> Retrns. counter increase
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmit; }

void TCPSender::send_empty_segment() {
    TCPSegment empty_tcpseg;
    empty_tcpseg.header().seqno = wrap(_next_seqno, _isn);  // Empty packet with correct sequence number
    _segments_out.push(empty_tcpseg);
}
