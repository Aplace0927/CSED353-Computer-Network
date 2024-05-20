Assignment 6 Writeup
=============

My name: Taeyeon Kim

My POVIS ID: taeyeonkim

My student ID (numeric): 20220140

This assignment took me about 3 hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): []

- **Caution**: If you have no idea about above best-submission item, please refer the Assignment PDF for detailed description.

Program Structure and Design of the Router:

- `RoutingTableElement`
  - `prefix_mask`: the prefix mask used when adding the route to routing table
  - `next_hop`, `interface_num`: argument from `add_route` function

- Adding entry to router

  - `mask` could be calculated by shifting `0xFFFFFFFF` to the right and invert.
    We can obtain the mask with its MSB filled with 1s.

  - If specific entry is not in routing table, then add it.

  - If superset of specific entry is in routing table, then update it.

    - We can determine whose entry is superset of other, by comparing mask.
      Because the longer mask value increases as mask length increases.

- Routing a single datagram

  - We should find the longest-match one, so mask the datagram's destination
    from the narrowest mask to the widest mask while search on the routing table.

    Starting from `0xFFFFFFFF` and bitshifting to the left with explicitly
    truncating to 32 bits, bitmask sequence could be obtained.

  - If any entry matching to the datagram's destination had found, then send
    the datagram via interface specified on routing table to following manner.

    - If there is information of next hop, according to `RoutingTableElement`
      corresponding to the entry, then send the datagram to that next hop.

    - If the next hop is unavailable, then send the datagram to the destination
      specified on the datagram directly.


  - When sending the datagram, we should update the `ttl` field of the datagram
    by decrementing it by 1.

    - If `ttl` field is 1, then drop the datagram.


Implementation Challenges:
When implementing TTL, I miss-implemented the condition of packet drop - 
Drop packet when TTL is 0. But packet should be dropped when TTL is 1.

Remaining Bugs:
Every testcase passed.