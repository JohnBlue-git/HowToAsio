# `io_context` vs `strand`

This comparison shows the difference between a plain executor and a serialized executor.

## Core idea

A plain `io_context` is the basic scheduler. It can run handlers on multiple threads when `run()` is invoked from multiple threads, and handlers are not automatically serialized.

A `strand` introduces ordering guarantees: all handlers associated with the same strand execute in sequence.

## Text graph

```text
Plain io_context path
---------------------
main thread
   |
   +-- post(task1)
   +-- post(task2)
   +-- post(task3)
   v
2 worker threads call io.run()
   |
   +-- task1 start
   +-- task2 start
   +-- task3 start
   |
   +-- overlapping execution possible
   v
shared state may race if handlers touch the same object

Strand path
-----------
main thread
   |
   +-- post(strand, task1)
   +-- post(strand, task2)
   +-- post(strand, task3)
   v
io_context with 2 workers
   |
   +-- strand serializes task execution
   +-- task1 -> task2 -> task3
   v
shared state stays ordered and non-overlapping
```

## Example behavior

The sample program schedules several tasks onto:

- a plain `io_context`
- a `strand`

The main difference is that the plain version can overlap, while the strand version is serialized.

## Why this matters

When multiple asynchronous handlers touch the same resource, the plain `io_context` model may allow race conditions. The strand model is a common fix for that by preserving a single execution order for the protected state.

## Rule of thumb

- use plain `io_context` for independently scheduled work
- use a `strand` when handlers share mutable state or require ordering

## See also

- API overview: [../../docs/boost_asio.md](../../docs/boost_asio.md)
- Design discussion: [../../docs/boost_asio_design.md](../../docs/boost_asio_design.md)
- Source: [io_context_vs_strand.cpp](io_context_vs_strand.cpp)
