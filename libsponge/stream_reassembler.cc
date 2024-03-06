#include "stream_reassembler.hh"

#include <iostream>
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
    /**
     * Ignore far-future packet (that cannot be stored on buffer) OR
     * Ignore already assembled packet
     */
    cout << "NOW STREAM STATUS: UNREAD: " << _output.bytes_read() << " UNASM: " << _output.bytes_written() << endl;
    if (index >= _output.bytes_read() + _capacity || index + data.length() <= _output.bytes_written()) {
        return;
    }

    /* EOF packet found*/
    if (eof) {
        cout << "EOF FOUND @ " << index + data.length() << endl;
        _eof_at = index + data.length();
    }

    /* Trim out unacceptable / already read substrings - for extreme cases*/
    Datagram curr(index, data);
    if (curr.from() < _output.bytes_read() + _capacity and _output.bytes_read() + _capacity <= curr.to()) {
        curr._data = curr._data.substr(0, max(0UL, _output.bytes_read() + _capacity - curr.from()));
    }
    if (curr.from() < _output.bytes_written() and _output.bytes_written() <= curr.to()) {
        curr._data = curr._data.substr(_output.bytes_written() - curr.from());
        curr._from = _output.bytes_written();
    }

    cout << "PUSHED & TRIMMED: " << curr.from() << " ~ " << curr.to() << " " << curr._data << endl;
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

    auto datagram_iter_max =
        [](std::set<StreamReassembler::Datagram>::iterator it1,
           std::set<StreamReassembler::Datagram>::iterator it2) -> std::set<StreamReassembler::Datagram>::iterator {
        return (std::distance(it1, it2) > 0) ? it2 : it1;
    };

    auto datagram_iter_min =
        [](std::set<StreamReassembler::Datagram>::iterator it1,
           std::set<StreamReassembler::Datagram>::iterator it2) -> std::set<StreamReassembler::Datagram>::iterator {
        return (std::distance(it1, it2) > 0) ? it1 : it2;
    };

    std::set<StreamReassembler::Datagram>::iterator lower_index = datagram_iter_max(
        _datagram_arrived.begin(), _datagram_arrived.lower_bound(Datagram(curr.from(), std::string("")))--);
    std::set<StreamReassembler::Datagram>::iterator upper_index = datagram_iter_min(
        _datagram_arrived.begin(), _datagram_arrived.upper_bound(Datagram(curr.to(), std::string("")))++);

    for (std::set<StreamReassembler::Datagram>::iterator target = lower_index; target != upper_index; target++) {
        /**
         * `target` datagram does not have intersection with recieved datagram
         */
        if (target->to() < curr.from() or curr.to() < target->from()) {
            continue;
        }

        if (curr.from() < target->from() and curr.to() <= target->to()) {
            curr._data = curr._data.substr(0, target->from() - curr.from()) + target->_data;
        } else if (target->from() <= curr.from() and target->to() < curr.to()) {
            curr._data = target->_data + curr._data.substr(target->to() - curr.from());
        } else if (target->from() <= curr.from() and curr.to() <= target->to()) {
            curr._data = target->_data;
        }
        curr._from = min(curr.from(), target->from());

        _unreasm -= target->len();
        cout << "REMOVING " << target->from() << " ~ " << target->to() << " " << target->_data << endl;
        _datagram_arrived.erase(target);
    }

    _unreasm += curr.len();
    cout << "INSERTING " << curr.from() << " ~ " << curr.to() << " " << curr._data << endl;
    _datagram_arrived.insert(curr);

    while (_datagram_arrived.empty() == false and _datagram_arrived.begin()->from() == _output.bytes_read()) {
        cout << "WRITE " << _datagram_arrived.begin()->from() << " ~ " << _datagram_arrived.begin()->to() << " "
             << _datagram_arrived.begin()->_data << endl;
        _output.write(_datagram_arrived.begin()->_data);
        cout << "NOW STREAM STATUS: UNREAD: " << _output.bytes_read() << " UNASM: " << _output.bytes_written() << endl;
        _datagram_arrived.erase(_datagram_arrived.begin());
    }

    /* Assemble until EOF found */
    if (_eof_at == _output.bytes_written() and _eof_at != 0) {
        cout << "END INPUT @ " << _eof_at << endl;
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unreasm; }

bool StreamReassembler::empty() const { return _datagram_arrived.empty(); }
