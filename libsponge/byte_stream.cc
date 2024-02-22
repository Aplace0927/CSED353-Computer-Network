#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) {
    _is_writing = false;
    _buffer = std::string("");
    _capacity = capacity;
    _remaining_capacity = 0U;
    _bytes_written = 0U;
    _bytes_read = 0U;
}

size_t ByteStream::write(const string &data) {
    int write_length = std::min<std::size_t>(data.length(), _remaining_capacity);

    try {
        _is_writing = true;
        _buffer.append(data.substr(0U, write_length));
    } catch (const std::exception &e) {
        return update_write(0U);
    }
    return update_write(write_length);
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    DUMMY_CODE(len);
    return {};
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { DUMMY_CODE(len); }

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    DUMMY_CODE(len);
    return {};
}

void ByteStream::end_input() {
    _is_writing = false;
    return;
}

bool ByteStream::input_ended() const { return {}; }

size_t ByteStream::buffer_size() const { return _capacity; }

bool ByteStream::buffer_empty() const { return _capacity == _remaining_capacity; }

bool ByteStream::eof() const { return false; }

size_t ByteStream::bytes_written() const { return {}; }

size_t ByteStream::bytes_read() const { return {}; }

size_t ByteStream::remaining_capacity() const { return _remaining_capacity; }

size_t ByteStream::update_read(size_t length_read) {
    _bytes_read += length_read;
    _remaining_capacity -= length_read;

    return length_read;
}

size_t ByteStream::update_write(size_t length_written) {
    _bytes_written += length_written;
    _remaining_capacity += length_written;

    return length_written;
}