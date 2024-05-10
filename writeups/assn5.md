Assignment 5 Writeup
=============

My name: Taeyeon Kim

My POVIS ID: taeyeonkim

My student ID (numeric): 20220140

This assignment took me about 6 hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): []

- **Caution**: If you have no idea about above best-submission item, please refer the Assignment PDF for detailed description.

Program Structure and Design of the NetworkInterface:

- Classes
  
  - `ARPMapping`
    - Stores ethernet address and time-to-live (TTL).
    - Has a method to determine whether `ARPMapping` had expired.

  - `IPSentInfo`
    - Stores IPv4 address and its datagrams to be sent.

- Datastructures
  
  - `std::map<uint32_t, ARPMapping> arp_table`
    - Stores mapping between IPv4 addr and (Ethernet address and its timeout)
  
  - `std::map<uint32_t, time_t> queried_arp`
    - Stores queried IPv4 addr and its queried time to check its validness.
    - Removes from map if specific entry had expired.

  - `std::vector<IPSentInfo> queried_ip`
    - Stores (Ethernet address, Datagram to be sent).
    - Removes from vector if specific entry had sent.
    
- Datagram Sending
  
  - IPv4 address argument had passed, and...
    
    - Had IPv4 address cached ARP Table?
      
      - Then fetch corresponding ethernet address, and send datagram to there.

    - Had IPv4 address queried to get Ethernet address?

      - If not, query ethernet address via `ARPMessage::ARP_REQUEST` packet.

    - If both are not, query the address and save the mapping to the ARP table.

    - After mapping, send ethernet frame crafted with corresponding data.

- Frame Receiving

  - Check the validness of the frame.

  - If packet is IPv4 type:

    - Just parse the packet and return the data.

  - If packet is ARP type:

    - If opcode is `ARP_REQUEST`:
    
      - Make a datagram to reply the packet.
    
    - If opcode is `ARP_REQUEST` or `ARP_REPLY`:

      - Create a mapping between IPv4 and ethernet address.
      - Send every packet in `queried_ip` that matches with sender's IPv4 address

- Timer count

  - Every function call, check the ARP table and remove expired entries.

Implementation Challenges:
Tried to implement removing expired element from STL containers with lambda functions.


Remaining Bugs:
Every testcase passed.