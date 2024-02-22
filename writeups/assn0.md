Assignment 0 Writeup
=============

My name: Taeyeon Kim

My POVIS ID: taeyeonkim

My student ID (numeric): 20220140

This assignment took me about 3 hours to do (including the time on studying, designing, and writing the code).

My secret code from section 2.1 was: `eff7f69d53`

- Optional: I had unexpected difficulty with:

    At the first glance, I tried to set up the environment with WSL 2 (on Ubuntu 22.04 LTS). When I was building initial baseline code
    with `g++-13` (the newest standard), it raised compilation error. However, modifying symbolic link to `g++` to `g++-11` (slightly older one),
    there was no compilation errors. I heard that this lab requires far modern C++ standard, the environment setting was hard for me.

- Optional: I think you could make this lab better by: 

    Applying `std::mutex` (or `std::semaphore`) on `ByteStream`, to ensure mutual exclusiveness between reader and writer
    will make this lab better. Current implementation uses boolean flag `_is_writing`, which is **STRICTLY** requires
    single-threaded environment (as remarked in the 7th page of document).

    Regarding any code that accesses to the `std::string _buffer` in this lab as a critical section. To control accessing
    to the `_buffer`, wrap the accessing part in `read`, `write`, `peek_output`, `pop_output` with suggested synchronization variables.
    (e.g. `std::mutex::lock` or `std::semaphore::acquire` when get in / `std::mutex::unlock` or `std::semaphore::release` when leave)

    Moreover, according to current implementation, writer cannot 'write' to the buffer once writer finishes writing
    (and call `end_input` to switch the mode). By applying synchronization variables and letting them to control mode,
    functions like `end_input` are no longer required, and also writer can 'write' again after reader finishes reading.