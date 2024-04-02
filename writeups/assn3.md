Assignment 3 Writeup
=============

My name: Taeyeon Kim

My POVIS ID: taeyeonkim

My student ID (numeric): 20220140

This assignment took me about 7 hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): []

- **Caution**: If you have no idea about above best-submission item, please refer the Assignment PDF for detailed description.

Program Structure and Design of the TCPSender:
- Variables
  
  - Modified default recieve window size to 1 (must bigger than 0).
  - New queue for managing packets waiting for acknowledgements (as
    known as "outstanding" packets)
    `std::queue<TCPSegment> _segments_outstand` 

  - New structure for manage timer to measure whether packets had
    acknowledged after RTO. - `Timer _timer`

- "Evolution" of TCP Sender - Designing TCP FSM

  Following the FSM diagram suggested on 7th page of document,
  I modeled each state as a functions with condition given.

    - `TCPSender::_status_closed` : Not sent SYN yet.
    - `TCPSender::_status_syn_sent`: SYN sent, nothing ACKed back yet.
    - `TCPSender::_status_syn_acked`: Stream ongoing.
    - `TCPSender::_status_syn_acked_reached_eof`: Met EOF, raise FIN.
    - `TCPSender::_status_fin_sent`: FIN sent, not ACKed to FIN yet.
    - `TCPSender::_status_fin_acked`: FIN acked, stream terminates.

- Timer management

  structure `Timer` was defined inside of `TCPSender`. Manages clock ticks
  to determine and manage whether specific packet did not ACKed after RTO.

  - Methods and Variables

    - Variables
      - `TCPSender::Timer::elapsed_ticks`: Accumulates tick elapsed.
      - `TCPSender::Timer::rto`: Threshold RTO.
      - `TCPSender::Timer::consec_retransmit`: Consecutive retransmit count.
        This value is not related to timer, but accumulated when `rto` has
        doubled.
      - `TCPSender::working`: Whether timer had activated or not.

    - Methods
      - `TCPSender::Timer::restart`: Restarts timer. (with activation)
      - `TCPSender::Timer::reset`: Reset the timer with given RTO.
      - `TCPSender::Timer::timeover`: If timer is active, return whether
        timer has elapsed over RTO.

  - Initializate

    Initialized when `TCPSender` is constructed, with given initial RTO value.

  - Activation
    
    `TCPSender::_send_tcp_segment` activates timer with sending TCP segment.
    (**Activating timer does NOT resets timer** - to track RTT between very
    first packet's RTT and on.)
    This function is called more than once in `TCPSender::fill_window` to send
    packet by chopping buffer up to `TCPConfig::MAX_PAYLOAD_SIZE` bytes.

  - Reset & Restart

    `TCPSender::ack_received` resets RTO into initial value (from construction).
    This function deques outdated packets (earlier than current ackno) in
    `_segments_outstand` queue.
    After dequeing, it resets RTO value and determine whether restart or
    deactivate counter by observing packets in the queue.

  - Increasing RTO

    `TCPSender::tick` manages timer expiration. Packets remaining on
    `_segment_outstand` queue when timer expired are not ACKed after
    retransmission timeout. Double the RTO, increase the "Consecutive-
    retransmission" counter by one, and restart timer with updated
    threshold.

- Filling window
  
  SYN packet must be sent and acknowledged to initiate the stream.
    - If SYN packet is not sent yet, send SYN packet.
    - If SYN packet is sent but not acknowledged, wait for ACK.

  After acknowledged SYN packet, send data packets until EOF found.
  When EOF found, send FIN packet to terminate the stream.
    - Fill the window, whose size was determined by previous ACK
      with `window_size`. Sum of 'in_flight' packet size must not exceed it,
      to maintain congestion control.

    - If packet is too big to send in single packet, chop it into pieces
      with `TCPConfig::MAX_PAYLOAD_SIZE` bytes.

  If FIN packet is sent, nothing to send anymore, returns.

- Acknowledgement received

  - When ACK received, update the window size with given `window_size` for
    congestion control.

  - If packet with specific `ackno` had received, remove every packets in
    `_segments_outstand` queue with `seqno` less than `ackno`.

  - Since packets are ACKed, "in-flight" packets are decreased. Maintaining
    the efficiency of congestion control, fill the recieve window by sending
    new packets. Reset the timer, and disable timer if no packets remaining.

Implementation Challenges:
- I implemented the timer inside of `TCPSender`, but I did not initialize the
  initial RTO value, so I was not able to pass specific test case scenario of
  SYN packet retransmission.

- Managing the TCP stream status without FSM states was hard to maintain the
  status of the stream, and also it led to error. I refactored the code to
  maintain TCP stream status with some status.

Remaining Bugs:
No bugs (Testcase all passed)

- Optional: I think you could make this assignment better by: 
The congestion control "window size" is not consistent. It might vary,
not constantly defined as a single value on receiving acknowledgment.
Implementing the congestion control algorithm to dealing with possible
packet loss might be a good idea to improve the assignment.