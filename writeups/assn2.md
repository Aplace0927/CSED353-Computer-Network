Assignment 2 Writeup
=============

My name: Taeyeon Kim

My POVIS ID: taeyeonkim

My student ID (numeric): 20220140

This assignment took me about 2 hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): []

- **Caution**: If you have no idea about above best-submission item, please refer the Assignment PDF for detailed description.

Program Structure and Design of the TCPReceiver and wrap/unwrap routines:

- Routines for wrapping / unwrapping

  - Wrapping integer

    Wrapping integer is converting 64-bit integer, offset with `isn`'s seqno.
    Therefore just add as 64-bit integer and bitmask to get least significant
    32-bit integer. (Of course, C++ semantics use `static_cast`)

  - Unwrapping Integer

    Once took the most significant 32bit of the checkpoint, absolute sequence
    number's most significant 32bit may differ -1, 0, or 1.
    Then get upper-dword of 64bit integer by bitmasking and calculate difference
    with differnce -1, no difference and difference 1.
    Take smallest difference 64bit offset among them, which is the absolute seqno.

- Design and routines of the TCPReceiver
  
  - `segment_received`

    Private flag variable `bool _syn` raises on the beginning of the stream:
    initialized with `false`, took or (`|`) operation on each `segment_received`
    function calls, with each segment header's `syn` field. 
    When the beggining had detected, `_isn` number initialized with its
    random 32-bit `seqno`, and available to push received data into its
    internal reassembler.

    To memorize the index of last input, `_chkpoint` accumulates the
    segment length of packet.

    Finally it pushes data into reassembler with its unwrapped stream index.
    As a notice, beggining packet's seqno should be increased by 1 -
    because of the `SYN` byte.
  
  - `ackno`

    Function returns `optional<T>`, then it should return `nullopt` until its
    acknowledgement number is determined, with observing `_syn` flag.

    If the `_syn` flag became true, the first unassembled byte's offset will
      * `+ 1`, for the `SYN` byte.
      * `+ stream_out().bytes_written()`, the asssembled and written data.

    Moreover, if the input ends, additional `FIN` byte must be counted.
    Implemented with typecasting boolean value into `uint64_t`, and add it.

  - `window_size`

    According to assignment #1, first unacceptable byte is defined as
    `(byte read) + (capacity)`, first unassembled byte is equal to 
    `(byte written into stream)`.
    Since both stream and reassembler are initialized with same size of
    capacity,`(total window size)` = `(capacity) - (written) + (read)`,
    is the definition of the `remaining_capacity` for the stream.  
    

Implementation Challenges:
Without awareness, I missed the existence of `FIN` byte on implementing
`ackno` parts. On my debugging, comparing the answer I've got and testcase
had differed by constant 1 on the very last, and I remembered the `FIN` byte.

Remaining Bugs:
No bugs (Testcase all passed)

- Optional: I was surprised by:
TCP uses 32bit number to represent sequence number, and its initial offset
is determined randomly with `SYN`. But I'm also not sure why it should be
determined as random number.