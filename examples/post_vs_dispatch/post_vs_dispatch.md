# `post` vs `dispatch`

This example highlights the execution-policy difference between deferring work and running it immediately when already in the right context.

## Text graph

```text
Inside a strand handler
   |
   +-- dispatch(strand, work)
   |        |
   |        +-- already on strand -> runs immediately
   |        +-- not on strand -> queued for later
   |
   +-- post(strand, later_task)
            |
            +-- always queued
            +-- runs after the current handler yields

Result:
- dispatch keeps the current execution flow tight
- post creates a clear queue boundary
```

## `post`

`boost::asio::post` always queues the handler for later execution. This is the safer default when you want to hand work to the executor and keep the current call stack simple.

## `dispatch`

`boost::asio::dispatch` checks whether the current thread is already running on the target executor. If yes, it runs immediately; otherwise it behaves like a queued handoff.

## Why this matters

This difference affects ordering, latency, and code structure. In practice:

- `post` is a clean way to defer work
- `dispatch` is useful when you are already on the correct strand or executor and want to avoid queueing overhead

## See also

- API overview: [../../docs/boost_asio.md](../../docs/boost_asio.md)
- Design discussion: [../../docs/boost_asio_design.md](../../docs/boost_asio_design.md)
- Source: [post_vs_dispatch.cpp](post_vs_dispatch.cpp)
