#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity), _capacity(capacity), _datagram_arrived() {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    /* Ignore far-future packet (that cannot be stored on buffer)*/
    if (index >= _output.bytes_read() + _capacity) {
        return;
    }

    /* EOF packet found*/
    if (eof) {
        _eof_at = index + data.length();
    }

    /* Trim out unacceptable / already read substrings - for extreme cases*/
    Datagram curr(index, data);
    if (curr.from() < _output.bytes_read() + _capacity and _output.bytes_read() + _capacity <= curr.to()) {
        curr._data = curr._data.substr(0, max(0UL, _output.bytes_read() + _capacity - curr.from()));
    }
    if (curr.from() < _output.bytes_read() and _output.bytes_read() <= curr.to()) {
        curr._data = curr._data.substr(_output.bytes_read() - curr.from());
        curr._from = _output.bytes_read();
    }

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

    while (_datagram_arrived.empty() == false and _datagram_arrived.begin()->from() == _output.bytes_read()) {
        _output.write(_datagram_arrived.begin()->_data);
        _datagram_arrived.erase(_datagram_arrived.begin());
    }

    /* Assemble until EOF found */
    if (_eof_at == _output.bytes_read() and _eof_at != 0) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return {}; }

bool StreamReassembler::empty() const { return {}; }
