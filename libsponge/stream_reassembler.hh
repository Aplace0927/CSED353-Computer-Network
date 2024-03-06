#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <set>
#include <string>
#include <vector>

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    class Datagram {
        /**
         * Defining datagram: storing its index and data as a single class.
         */
      public:
        size_t _from;       // Starting index
        std::string _data;  // Actual data

        /**
         * Constructor of `Datagram`.
         * With its starting index and actual data
         */
        Datagram(size_t from, std::string data) : _from(from), _data(data) {}

        /**
         * Interface functions to access
         * string properties when stitching.
         */
        size_t from() const { return _from; }
        size_t to() const { return _from + _data.length(); }
        size_t len() const { return _data.length(); }

        /**
         * Operator overloaded for managing `Datagram` with
         *  `std::set`, which is internally sorted using red-black tree.
         */
        friend bool operator<(const Datagram &d1, const Datagram &d2) { return d1._from < d2._from; }
    };
    size_t _eof_at = static_cast<size_t>(
        -1);  // "Indicating" eof not recieved as static casting -1 (Bit pattern would be 0xFFFF`FFFF`FFFF`FFFF)
    size_t _unreasm = 0UL;  // Initially zero bytes had reassembled

    ByteStream _output;  //!< The reassembled in-order byte stream
    size_t _capacity;    //!< The maximum number of bytes

    /**
     * Managing recieved datagrams using `std::set`,
     * order defined by overloaded `operator<`
     *
     * The smaller index of `Datagram::_from` will place
     * `Datagram` at the beggining side of `std::set`
     */
    std::set<Datagram> _datagram_arrived;

  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receive a substring and write any newly contiguous bytes into the stream.
    //!
    //! The StreamReassembler will stay within the memory limits of the `capacity`.
    //! Bytes that would exceed the capacity are silently discarded.
    //!
    //! \param data the substring
    //! \param index indicates the index (place in sequence) of the first byte in `data`
    //! \param eof the last byte of `data` will be the last byte in the entire stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been pushed more than once, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
