# Boost.Asio Fundamentals

Boost.Asio is a C++ library for asynchronous I/O and concurrency. It is widely used for sockets, timers, polling, background tasks, and event-driven service code. The library is template-heavy and large, so there is no single fixed function count. In practice, you work with a compact set of core concepts and a fairly small number of functions that matter in day-to-day use.

> In short: Asio is not “one function.” It is an execution model built around executors, contexts, handlers, and async operations.

## 1. What Asio is

Asio provides a consistent way to do asynchronous operations without blocking the program. The most important objects are:

- `io_context`: the scheduler / event loop
- sockets and timers: the async resources
- handlers: callback functions or coroutine continuations
- executors and strands: execution policy and serialization

The library is available as Boost.Asio and also as standalone Asio, but the concepts are the same.

## 2. Essential Asio function families

### 2.1 Execution and context management

These functions drive the event loop and submit work to the execution context.

- `io_context::run()` — blocks until there is no more work to do
- `io_context::stop()` — stops the loop early
- `asio::post()` — schedules work to the execution context
- `asio::dispatch()` — executes immediately if already in the right context, otherwise queues work
- `asio::defer()` — defers work in a way consistent with the target executor

### 2.2 Stream I/O free functions

These are the free functions that operate on streams and guarantee that the requested amount of data is processed before returning.

- `asio::read()` / `asio::write()`
- `asio::async_read()` / `asio::async_write()`
- `asio::read_until()` / `asio::async_read_until()`

These are the classic APIs for reading exact-length payloads or delimited text such as HTTP headers.

### 2.3 Socket member functions

These are called on socket objects such as `boost::asio::ip::tcp::socket`.

- `socket.connect()` / `socket.async_connect()`
- `socket.close()`
- `socket.read_some()` / `socket.async_read_some()`
- `socket.write_some()` / `socket.async_write_some()`

These are useful when the application is handling partial reads and writes, not fixed-length protocol messages.

### 2.4 Buffering

- `asio::buffer()` — wraps arrays, strings, vectors, or memory ranges into a form Asio can read or write

This is the core mechanism for passing data between application code and socket operations.

### 2.5 Timers and delayed work

- `steady_timer`
- `deadline_timer`
- `async_wait()`

These are used heavily for delayed work, watchdogs, or periodic operations.

### Example code for the core function families

#### A. `io_context::run()`, `io_context::stop()`, `post()`, `dispatch()`, and `defer()`

```cpp
#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <thread>

int main() {
    boost::asio::io_context io;

    boost::asio::post(io, [] {
        std::cout << "post() -> queued for the event loop\n";
    });

    boost::asio::dispatch(io.get_executor(), [] {
        std::cout << "dispatch() -> runs in the current executor context\n";
    });

    boost::asio::defer(io, [] {
        std::cout << "defer() -> schedules a later callback\n";
    });

    std::thread worker([&] {
        io.run();
        std::cout << "run() finished\n";
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    io.stop();
    worker.join();
}
```

This shows the lifecycle pattern clearly: work is posted or dispatched, the loop runs, and `stop()` ends it early when needed.

#### B. `asio::read()` / `asio::write()` and `asio::async_read()` / `asio::async_write()`

```cpp
#include <boost/asio.hpp>
#include <array>
#include <iostream>
#include <string>

int main() {
    boost::asio::io_context io;
    boost::asio::ip::tcp::socket socket(io);

    std::string payload = "hello\n";
    std::array<char, 64> buffer{};

    // Assume the socket is already connected.
    boost::asio::write(socket, boost::asio::buffer(payload));
    std::size_t n = boost::asio::read(socket, boost::asio::buffer(buffer));

    std::cout << "received: " << std::string(buffer.data(), n) << "\n";

    boost::asio::async_write(socket, boost::asio::buffer(payload),
        [](const boost::system::error_code& ec, std::size_t /*bytes_sent*/) {
            if (!ec) {
                std::cout << "async_write completed\n";
            }
        });

    boost::asio::async_read(socket, boost::asio::buffer(buffer),
        [](const boost::system::error_code& ec, std::size_t bytes_read) {
            if (!ec) {
                std::cout << "async_read got " << bytes_read << " bytes\n";
            }
        });

    io.run();
}
```

The synchronous APIs block until the transfer completes. The asynchronous versions return immediately and call the completion handler later when the I/O operation finishes.

#### C. `asio::read_until()` / `asio::async_read_until()`

```cpp
#include <boost/asio.hpp>
#include <array>
#include <iostream>
#include <string>

int main() {
    boost::asio::io_context io;
    boost::asio::ip::tcp::socket socket(io);
    std::array<char, 256> buffer{};

    // Assume the socket is already connected and receives "line\r\n".
    boost::asio::read_until(socket, boost::asio::dynamic_buffer(buffer), "\r\n");

    boost::asio::async_read_until(socket, boost::asio::dynamic_buffer(buffer), "\r\n",
        [](const boost::system::error_code& ec, std::size_t bytes_read) {
            if (!ec) {
                std::cout << "read_until read " << bytes_read << " bytes\n";
            }
        });

    io.run();
}
```

This is the classic API for parsing delimited text such as HTTP headers or line-oriented network protocols.

#### D. `socket.connect()` / `socket.async_connect()`, `socket.close()`, `socket.read_some()` / `socket.async_read_some()`, and `socket.write_some()` / `socket.async_write_some()`

```cpp
#include <boost/asio.hpp>
#include <array>
#include <iostream>

int main() {
    boost::asio::io_context io;
    boost::asio::ip::tcp::socket socket(io);
    boost::asio::ip::tcp::endpoint endpoint(
        boost::asio::ip::make_address("127.0.0.1"), 8080);

    socket.connect(endpoint);

    std::array<char, 64> read_buf{};
    std::string payload = "ping";

    std::size_t sent = socket.write_some(boost::asio::buffer(payload));
    std::size_t received = socket.read_some(boost::asio::buffer(read_buf));

    std::cout << "sent=" << sent << ", received=" << received << "\n";

    socket.async_connect(endpoint, [](const boost::system::error_code& ec) {
        if (!ec) {
            std::cout << "async_connect succeeded\n";
        }
    });

    socket.async_write_some(boost::asio::buffer(payload),
        [](const boost::system::error_code& ec, std::size_t bytes) {
            if (!ec) {
                std::cout << "async_write_some wrote " << bytes << " bytes\n";
            }
        });

    socket.async_read_some(boost::asio::buffer(read_buf),
        [](const boost::system::error_code& ec, std::size_t bytes) {
            if (!ec) {
                std::cout << "async_read_some got " << bytes << " bytes\n";
            }
        });

    socket.close();
    io.run();
}
```

Socket member functions are the direct per-socket form of the stream APIs. They are useful when the application is working with partial reads and writes instead of a full fixed-length message.

#### E. `asio::buffer()`

```cpp
#include <boost/asio.hpp>
#include <array>
#include <iostream>
#include <string>

int main() {
    std::string text = "hello";
    std::array<char, 16> storage{};

    auto text_view = boost::asio::buffer(text);
    auto storage_view = boost::asio::buffer(storage);

    std::cout << "text view size: " << text_view.size() << "\n";
    std::cout << "storage view size: " << storage_view.size() << "\n";
}
```

`buffer()` is the glue layer between raw memory and Asio. It accepts arrays, strings, vectors, and other contiguous ranges and exposes them in the form required by Asio I/O calls.

#### F. `steady_timer` / `deadline_timer` / `async_wait()`

```cpp
#include <boost/asio.hpp>
#include <chrono>
#include <iostream>

int main() {
    boost::asio::io_context io;

    boost::asio::steady_timer timer1(io, std::chrono::milliseconds(100));
    timer1.async_wait([](const boost::system::error_code& ec) {
        if (!ec) {
            std::cout << "steady_timer fired\n";
        }
    });

    boost::asio::deadline_timer timer2(io, boost::asio::chrono::milliseconds(200));
    timer2.async_wait([](const boost::system::error_code& ec) {
        if (!ec) {
            std::cout << "deadline_timer fired\n";
        }
    });

    io.run();
}
```

Timers are one of the simplest and most common Asio building blocks. They allow the program to schedule delayed work without blocking the thread.

These examples are intentionally grouped to show that most Asio code follows the same pattern: schedule async work, wait for readiness, and handle the result in a callback or coroutine continuation.

## 3. Text graph of the event loop

```text
App code
   │
   ├── post() / async_read() / async_connect() / async_wait()
   ▼
io_context
   │
   ├── queues ready handlers
   ├── waits for OS readiness events
   ▼
handler callback / coroutine continuation
   │
   └── application logic runs
```

The mental model is simple: schedule work, wait for readiness, then run the handler or coroutine continuation.

## 4. The core execution model

### 4.1 `io_context`

The `io_context` is the central scheduler. It owns pending async work and dispatches handlers when they become ready.

```cpp
#include <boost/asio.hpp>
#include <iostream>

int main() {
    boost::asio::io_context io;

    boost::asio::post(io, [] {
        std::cout << "hello from Asio\n";
    });

    io.run();
}
```

`run()` blocks until there is no more work queued or active.

### 4.2 `post` vs `dispatch`

```cpp
boost::asio::post(io, [] {
    std::cout << "deferred\n";
});

boost::asio::dispatch(io.get_executor(), [] {
    std::cout << "immediate if already on the correct executor\n";
});
```

The difference is execution policy:

- `post` defers work
- `dispatch` avoids a queue hop when already in the right execution context

### 4.3 `strand`

A `strand` ensures that handlers associated with the same strand do not overlap.

```cpp
boost::asio::io_context io;
auto strand = boost::asio::make_strand(io);

boost::asio::post(strand, [] { std::cout << "A\n"; });
boost::asio::post(strand, [] { std::cout << "B\n"; });

io.run();
```

This is why `strand` is so common in stateful services: it serializes callback access without forcing every operation into one global lock.

### 4.4 `co_spawn`

Coroutines are useful when several async steps naturally form a sequence.

```cpp
boost::asio::co_spawn(
    io,
    [&]() -> boost::asio::awaitable<void> {
        boost::asio::steady_timer timer(io, std::chrono::milliseconds(50));
        co_await timer.async_wait(boost::asio::use_awaitable);
        std::cout << "resumed after wait\n";
        co_return;
    },
    [](std::exception_ptr ex) {
        if (ex) {
            try {
                std::rethrow_exception(ex);
            } catch (const std::exception& e) {
                std::cerr << e.what() << '\n';
            }
        }
    });
```

This style is especially helpful when async code reads naturally as a linear flow.

## 5. Core comparison table

| Concept | What it does | Typical use | Key trade-off |
| --- | --- | --- | --- |
| `io_context` | Schedules async work | Central event loop | Shared callbacks need careful design |
| `post` | Queues work | Deferred tasks | Slightly more latency |
| `dispatch` | Executes immediately if possible | Already-correct context | Requires the right executor |
| `strand` | Serializes handlers | Shared mutable state | Can reduce concurrency |
| `co_spawn` | Async sequential flow | Long async logic chains | More coroutine-specific complexity |

## 6. Why this matters in practice

The most common design mistake is to think that “async” automatically means “thread-safe.” It does not. Asio gives you scheduling, not protection for every object touched by every callback.

That is why the real design problem is usually:

- which execution context should own the work?
- which objects need serialized access?
- do we use callbacks or coroutines?

## 7. Related discussion

For execution-model design, read [boost_asio_design.md](boost_asio_design.md). For concrete comparison examples, see [examples/io_context_vs_strand/io_context_vs_strand.md](examples/io_context_vs_strand/io_context_vs_strand.md), [examples/post_vs_dispatch/post_vs_dispatch.md](examples/post_vs_dispatch/post_vs_dispatch.md), and [examples/post_vs_cospawn/post_vs_cospawn.md](examples/post_vs_cospawn/post_vs_cospawn.md).

## 8. Practical takeaway

The Asio learning path is:

1. understand `io_context`
2. understand async operations and buffers
3. understand `strand` and executor boundaries
4. choose `post`, `dispatch`, or `co_spawn` based on flow and ownership

That is the real basis of Asio programming.
