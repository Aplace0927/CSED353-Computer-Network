Assignment 1 Writeup
=============

My name: Taeyeon Kim

My POVIS ID: taeyeonkim

My student ID (numeric): 20220140

This assignment took me about 20 hours to do (including the time on studying, designing, and writing the code).

Program Structure and Design of the StreamReassembler:

- Member variable, class, functions

  - `class Datagram` 
    
    Added to manage datagram itself with size and data.
    For the ease of comparison, it provides its properties as 'getter' functions.
    
    Less than (`<`) operator was overridded to maintain its tree structure.
    `std::set` requires comparison operator overriding.

  - `size_t _eof_at` and `size_t _unreasm`

    `_eof_at` detects EOF. Initialized with `-1` and value changes when
    feasible value had inputted with `eof` flag.

    `_unreasm` counts un-assembled bytes.
    It increases when data is pushed into set, decreases when data is popped.
    
  - `std::set<Datagram> _datagram_arrived`

    Manage datagrams with `std::set` datastructure.
    `Datagrams` are sorted by increasing order of its starting index.

- How `StreamReassembler::push_substring` works

  - Detect whether given packet is EOF at the first stage,
    update `eof` location. Instantly terminate if EOF with empty datagram.

  - Filter meaningless data. Data that are already sent to `Bytestream` or
    overflow the capacity of `StreamReassembler`, empty data must be ignored.

  - Trim-out data. If current datagram has intersection with already sent data
    or overflowed part: trim-out them.

  - Iterate the current `std::set` with un-reassembled datagrams.
    Merge with other datagrams, as many as recieved datagram can merge.
    When datagram is merged, it should be removed from the `std::set`.

  - Insert the merged datagram into `std::set`, and export to the `Bytestream`
    if possible. (Last exported index should equal to head of set's start.)

  - After export to the `Bytestream`, compare the last exported index with
    EOF found (in the first stage of current function).
    If every bytes are sent to `Bytestream`, then terminate input.

Implementation Challenges:

- Managing intervals without intersection.

  It was a hard case-working, sensitive inequality
  between its starting and ending index.

- `lower_bound` and `upper_bound` did not work. 

  To find out proper index to push current datagram,
  I originally used `lower_bound` and `upper_bound` to
  maintain `O(log n)` time-complexity to find out proper
  index to push datagram.

  However, I was searching last index that its finish index
  is smaller than current datagram. They returned diffrent
  value. I finally spent `O(n)` to traverse with looking
  entire red-black tree.

Remaining Bugs:
No bugs (Testcase all passed)

- Optional: I had unexpected difficulty with: 
  
  - Understanding how `StringReassembler` works, and it led me to try
    lots of different data structures provided in C++ STL library.

    - Initially, I tried `std::priority_queue<std::pair<size_t, size_t>>`.
      Assuming **there are no overlapping substrings** - push the
      accepted interval into priority queue, and pop from the head
      of priority queue when it's possible.
    
    - After that, I misunderstood that the overlapping byte **may differ**
      even its index are same, tried to prioritize between conflicting
      datagrams by its index of arrival.
    
    - Finally, I found `std::set` or `std::map` templated with proper
      `Datagram` definition, including its start index and string. Standard
      template library internally use red-black tree to manage data.