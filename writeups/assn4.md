Assignment 4 Writeup
=============

My name: Taeyeon Kim

My POVIS ID: taeyeonkim

My student ID (numeric): 20220140

This assignment took me about 12 hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): []

- **Caution**: If you have no idea about above best-submission item, please refer the Assignment PDF for detailed description.

Your benchmark results (without reordering, with reordering): [4.06, 3.73]

Program Structure and Design of the TCPConnection:

- Wiring `TCPSender` and `TCPReceiver`

  - IF `TCPReceiver` got segment with `RST` flag":

    - Call `close(false)` to explicitly 'dirty'-close the connection.
      It is not necessary to count neither `ackno` nor `seqno`,
      because the connection will immediately closed with `RST` flag,
      with `launch(false)`.
  
  - IF `TCPReceiver` got segment without `RST` flag:

    - Then `TCPReceiver` will check the segment's `ackno` and `seqno`,
      to maintain the between server and our implementation.
    
  There are some case that `TCPReceiver` receives any segment. (w/o `RST` flag)
  
  1. Received packet is used on 3-way handshake.

    - Then the state of the connection is `SYN_RCVD`:
        * `TCPReceiver` is in `TCPReceiverStateSummary::SYN_RECV` state.
        * `TCPSender` is in `TCPSenderStateSummary::CLOSED` state.
      Sender should send `ACK` + `SYN` flag to establish the connection.

  2. Received packet is `ACK`ing to data we sent.

    - Then sender should provide next data packet.
      This job will done in `_sender.ack_received()` function.
    
    - However, if the server `ACK`ed to our last data:
      Yet we should send another next data for response to the server's `ACK`,
      so we should send empty segment to server (as a explicit end signal).
      (We had implemented serverside empty segment handling in `TCPReceiver`.)

  3. Keep-alive packet is received.

    - This packet does NOT occupy the seqno space.
      Also this packet could be handled by sending empty segment to server.
  
  Finally, do a 4-way handshake to close the connection.
  (At the last part of the connection, we sent `ACK` by `launch(false)` and
  `FIN` by `close(true)` to close the connection 'clean'-ly.)

- Helper function

  - `void TCPConnection::launch(const bool reset)`

    This function is used to send packet, by pushing packet to outbound queue.
    `TCPConnection` may send 2 kind of packet, without 3 or 4 way handshaking:
    
    - `RST` packet: always sent once.
    - `ACK` packet: should be sent for each packet in sender's `_segments_out`.

    Poth packet should be set its ackno to its corresponding calculated ackno,
    from `_receiver.ackno().value()`. Then do a simple congestion control with
    setting `win` field as much as can be.

  - `void TCPConnection::close(const bool graceful)`
    There are 2 ways to close the connection: 'clean' and 'dirty'.
    Parameter `graceful` is used to determine the way of closing connection.

    - 'Dirty' close

      Always be paired with `launch(true)`, because dirty closing needs to
      send `RST` packet before TCP connection's inbound & outbound stream
      raise ERROR state.
      This function raise stream as ERROR state, then deactivate the
      `TCPConnection`.

    - 'Clean' close

      After inboudnd stream had reached `EOF` before outbound stream,
      then we should set `TCPConnection` to 'lingering' state.
      This is because waiting any possible packet which are not
      arrived yet from server.
      We can finally deactivate the connection after the linger time had passed
      (Defined as 10 times of retransmission timeout).

Implementation Challenges:

I had implementation challenge with `close`, so I tried to debug
`tcp_connection.cc` but everything seemed to be fine.
In the testbench `t_passive_close`, my implementation could not sent
`FIN` packet after lingering time had passed. This was not a fault in
`tcp_connection` but `tcp_sender`.

For the FSM design of `TCPConnection`, retransmission timeout and timer working
should be resetted after `TCPSender` sends packet, and it eagers to wait any
response. (4-way handshaking packet does not wait response.)
But in my implementation, I reset retransmission timeout on every
packet sending. This was the reason why `FIN` packet was not sent -
retransmission timeout is steadily resetted again.

Remaining Bugs:

No bugs (Testcase all passed)

- Optional: I think you could make this assignment better by:

Suggesting how to profile our `TCPConnection` implementation.
I think there are some way to profile implementaion, but I used `perf` in
Linux kernel directory to profile `apps/tcp_benchmark`.

According to my result analyzed with `perf report`, following functions
are speculated to have significant overheads.
- `random` function.
- Memory allocator & deallocator functions.
  Internally calling `__mmap` and `__munmap` to allocate stream buffer space.