#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity), _capacity(capacity), _datagram_arrived() {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    /* EOF packet found*/
    if (eof) {
        _eof_at = index + data.length();
        if (data.length() == 0)  // Empty packet but EOF
        {
            _output.end_input();
            return;
        }
    }
    /**
     * Ignore far-future packet (that cannot be stored on buffer) OR
     * Ignore already assembled packet
     */
    if (index >= _output.bytes_read() + _capacity || index + data.length() <= _output.bytes_written() || data.empty()) {
        return;
    }

    /* Trim out unacceptable / already read substrings - for extreme cases*/
    Datagram curr(index, data);
    if (curr.from() < _output.bytes_read() + _capacity and _output.bytes_read() + _capacity <= curr.to()) {
        // Trim-out unacceptable substring
        curr._data = curr._data.substr(0, max(0UL, _output.bytes_read() + _capacity - curr.from()));
    }
    if (curr.from() < _output.bytes_written() and _output.bytes_written() <= curr.to()) {
        // Trim-out already read substring
        curr._data = curr._data.substr(_output.bytes_written() - curr.from());
    }
    curr._from = max(_output.bytes_written(), index);  // Trim-out starting index

    /**
     * TODO: Stitch Bytes
     *
     * If target has intersection with each `Datagram`s:
     *  - Covered by / Covers with / Left intersect / Right intersect
     *
     * THIS SEQUENCE MIGHT HAPPEN CONSEQUENTLY.
     *
     * Covered by: then ignore current datagram (already covered)
     * Covers with: remove covered datagram, push current.
     * Left-intr / Right intr:
     *      set its index to left one's index,
     *      set data as union of both datagrams
     */

    for (std::set<StreamReassembler::Datagram>::iterator target = _datagram_arrived.begin();
         target != _datagram_arrived.end();
         target++) {
        /**
         * `target` datagram does not have intersection with recieved datagram
         */
        if (target->to() <= curr.from() or curr.to() <= target->from()) {
            continue;
        }
        if (curr.from() < target->from() and curr.to() <= target->to()) {
            /**
             * Current datagram overlapps with target's starting side
             * -> Extend current data with target's unique substring
             */
            curr._data = curr._data.substr(0, target->from() - curr.from()) + target->_data;
        } else if (target->from() <= curr.from() and target->to() < curr.to()) {
            /**
             * Current datagram overlapps with target's ending side
             * -> Insert target's unique substring at the start of current data
             */
            curr._data = target->_data + curr._data.substr(target->to() - curr.from());
        } else if (target->from() <= curr.from() and curr.to() <= target->to()) {
            /**
             * Current datagram is covered by target.
             * -> Replace current with target.
             */
            curr._data = target->_data;
        }
        curr._from = min(curr.from(), target->from());  // Set start to beginning of intersection

        /**
         * Remove target (because it has stitched to current)
         * Erase from `std::set` with its iterator and decrease the reassembled length.
         */
        _unreasm -= target->len();
        _datagram_arrived.erase(target);
    }

    /**
     * After finish stitching,
     * Insert again into `std::set` and increase the reassembled length/
     */
    _unreasm += curr.len();
    _datagram_arrived.insert(curr);

    /**
     * Push assembled datagrams into `Bytestream`.
     * Byte written to the `Bytestream` must be equal to starting index of un-reassembled byte.
     */
    while (_datagram_arrived.empty() == false and _datagram_arrived.begin()->from() == _output.bytes_written()) {
        _output.write(_datagram_arrived.begin()->_data);
        _unreasm -= _datagram_arrived.begin()->len();  // Decrease un-reassembled byte - written to `Bytestream`
        _datagram_arrived.erase(_datagram_arrived.begin());
    }

    /* Assemble until EOF found */
    if (_eof_at == _output.bytes_written() and _eof_at != static_cast<size_t>(-1)) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unreasm; }

bool StreamReassembler::empty() const { return _datagram_arrived.empty(); }
