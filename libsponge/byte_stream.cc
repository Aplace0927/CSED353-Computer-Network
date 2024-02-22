#include "byte_stream.hh"

#include <iostream>
// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : _capacity(capacity), _remaining_capacity(capacity) {}

size_t ByteStream::write(const string &data) {
    const size_t write_length =
        std::min<std::size_t>(data.length(), remaining_capacity());  // Write as many as will fit

    try {
        _buffer.append(data.substr(0U, write_length));
    } catch (const std::exception &e) {
        return update_write(0U);  // If failed to append, return "nothing written"
    }
    return update_write(write_length);  // Update the number of bytes written and the remaining capacity and return
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    const size_t peek_length = std::min<std::size_t>(len, buffer_size());

    try {
        return _buffer.substr(0U, peek_length);  // Peek at next "len" bytes of the stream
    } catch (const std::exception &e) {
        return std::string("");  // If failed to peek, return an empty string
    }
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    const size_t read_length = std::min<std::size_t>(len, buffer_size());

    try {
        _buffer.erase(0, read_length);  // Remove bytes from the buffer
    } catch (const std::exception &e) {
        update_read(0U);  // If failed to erase, return "nothing read"
        return;
    }
    update_read(read_length);  // Update the number of bytes read and the remaining capacity
    return;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    const size_t read_length = std::min<std::size_t>(len, buffer_size());

    std::string read_data;

    try {
        read_data = _buffer.substr(0, read_length);  // Read as many as will fit
        _buffer.erase(0, read_length);               // Pop it
    } catch (const std::exception &e) {
        update_read(0U);         // If failed to read, return "nothing read" and
        return std::string("");  // Return an empty string
    }
    update_read(read_length);  // Update the number of bytes read and the remaining capacity
    return read_data;          // Return the read data
}

void ByteStream::end_input() {
    _is_writing = false;
    return;
}

bool ByteStream::input_ended() const { return _is_writing == false; }

size_t ByteStream::buffer_size() const { return _capacity - _remaining_capacity; }

bool ByteStream::buffer_empty() const { return _capacity == _remaining_capacity; }

bool ByteStream::eof() const { return input_ended() && buffer_empty(); }

size_t ByteStream::bytes_written() const { return _bytes_written; }

size_t ByteStream::bytes_read() const { return _bytes_read; }

size_t ByteStream::remaining_capacity() const { return _remaining_capacity; }

size_t ByteStream::update_read(size_t length_read) {
    // Update the remaining capacity and the total number of bytes read at once
    _bytes_read += length_read;
    _remaining_capacity += length_read;

    return length_read;
}

size_t ByteStream::update_write(size_t length_written) {
    // Update the remaining capacity and the total number of bytes written at once
    _bytes_written += length_written;
    _remaining_capacity -= length_written;

    return length_written;
}