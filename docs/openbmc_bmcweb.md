# OpenBMC bmcweb and Asio Design

This note describes the real bmcweb pattern, not an idealized Asio server. The main takeaway is that the actual OpenBMC bmcweb implementation uses a single shared `boost::asio::io_context` and then builds HTTP, D-Bus, and signal handling on top of that context.

## 1. The actual pattern in bmcweb

The actual singleton in the upstream project is very simple:

```cpp
inline boost::asio::io_context& getIoContext()
{
    static boost::asio::io_context io;
    return io;
}
```

This comes from the real bmcweb source and is the central scheduling object for the process.

The key architectural idea is not “many contexts, many strands everywhere.” It is: one service-wide event loop, with multiple subsystems registering async work on that loop.

## 2. Why the single context matters

In bmcweb, the same `io_context` is used for:

- HTTP socket accepts and async request handling
- D-Bus connection lifecycle and method calls
- signal handling
- timers and other deferred work

This matters because it keeps the server model coherent: one scheduler, multiple async subsystems, and a single place where the service is alive.

## 3. Real HTTP side shape

The actual server code creates signal handlers from the same `io_context` and then calls `doAccept()` to begin accepting client sockets.

The main pattern is:

```cpp
signals(getIoContext(), SIGINT, SIGTERM, SIGHUP)
```

and then:

```cpp
doAccept();
```

The server is not built around a page-wide custom event loop. It is built around Asio and its normal async interfaces.

This means the HTTP server pattern is conceptually:

- a socket acceptor waits asynchronously
- when a client connects, new work is scheduled
- the completion handler continues the request flow

This is exactly the standard Asio style for an HTTP service.

## 4. Real D-Bus side shape

The upstream bmcweb code creates the D-Bus Asio connection with the same shared `io_context`:

```cpp
boost::asio::io_context& io = getIoContext();

std::shared_ptr<sdbusplus::asio::connection> systemBus =
    std::make_shared<sdbusplus::asio::connection>(io);

crow::connections::systemBus = systemBus.get();
```

This is important: the D-Bus subsystem is not a separate scheduler. It is attached to the same event loop that drives the HTTP server.

That is a very common design for service code: all async subsystems share a single event loop, and the library or service code manages the scheduling internally.

## 5. What is not happening in the real bmcweb code

The real upstream code does not show a blanket “every HTTP connection gets a strand” design. The bmcweb design is simpler and more centralized than that.

So the following is not the real bmcweb architecture:

- one global `strand` for all HTTP work
- one `strand` per every object in the whole server
- a custom asynchronous threading model at the top level

Instead, the real design is: one `io_context`, with subsystem logic and library-level ordering imposed where appropriate.

This is a crucial distinction: a `strand` is an important tool, but it is not mandatory for every callback in every service.

## 6. Correct interpretation of `strand` in this context

If a service has a connection object with shared mutable state, then a `strand` is a good way to serialize access. But in bmcweb, the main design is not to globally wrap all work in `make_strand()`.

The more accurate interpretation is:

- bmcweb uses a single `io_context` for service-wide scheduling
- specific object-level or subsystem-level serialization may be layered on top when needed
- the primary architectural model is central event-loop coordination, not blanket strandification

## 7. Comparison table: idealized vs actual bmcweb pattern

| Pattern | Description | Real bmcweb behavior |
| --- | --- | --- |
| Single shared `io_context` | one scheduler for the app | Yes |
| One `strand` for everything | all handlers serialized globally | Not the main pattern |
| one `strand` per connection | local shared-state protection | plausible local optimization, not universal design |
| HTTP + D-Bus on one loop | one event loop across subsystems | Yes |

## 8. Why this is an important Asio lesson

bmcweb is an excellent example of a real service whose architecture is not “pure theory.” It is practical service design:

- the app chooses a unified event loop
- async subsystems register work against it
- the service uses library integration and callback-based patterns rather than a complex custom scheduler

This is the correct Asio mental model for many system daemons and network services.

## 9. Practical conclusion

For a study project, the best summary is:

- Asio gives the scheduler and async primitives
- bmcweb chooses one shared `io_context`
- HTTP and D-Bus are both registered into that loop
- `strand` is a useful local tool, but it is not the primary architecture of this real service

This is a much more accurate understanding than assuming every component in bmcweb is automatically wrapped in a custom `strand` design.

## 10. Related reading

- Fundamentals: [boost_asio.md](boost_asio.md)
- Design trade-offs: [boost_asio_design.md](boost_asio_design.md)
- Code-oriented comparisons: [examples/io_context_vs_strand/io_context_vs_strand.md](examples/io_context_vs_strand/io_context_vs_strand.md), [examples/post_vs_dispatch/post_vs_dispatch.md](examples/post_vs_dispatch/post_vs_dispatch.md), [examples/post_vs_cospawn/post_vs_cospawn.md](examples/post_vs_cospawn/post_vs_cospawn.md)
