# `post` vs `co_spawn`

This example demonstrates two different ways of turning asynchronous work into a runnable sequence:

- plain callback tasks via `post`
- coroutine tasks via `co_spawn`

The point is not that one is always better, but that they schedule and link work differently. In the example, we intentionally queue multiple `post` callbacks and then chain coroutine steps so you can see how work is added to the same `io_context` and processed in order.

## Text graph

```text
main thread
   |
   +-- post(task A)
   |      |
   |      +-- enqueue post(task B)
   |      |      |
   |      |      +-- enqueue post(task C)
   |      v
   +-- co_spawn(coro A)
          |
          +-- wait 30ms
          v
          resume and schedule coro B
                 |
                 +-- wait 20ms
                 v
                 resume and schedule coro C + post(task D)
                        |
                        +-- wait 10ms
                        v
                        final resume

All of this work is still being executed by one io_context.
```

## Callback-style queueing

`post` is a straightforward way to say: “run this later on the event loop.” If you call `post` from inside another callback, the new callback is simply inserted into the same work queue and will run later, after the current callback completes.

That is why this example creates a nested sequence such as:

```cpp
boost::asio::post(io, [&io] {
    std::cout << "post #1: first item in the queue\n";
    boost::asio::post(io, [&io] {
        std::cout << "post #2: scheduled from post #1\n";
    });
});
```

The outer callback is not executed in parallel with the nested callback. Instead, the nested callback is posted for later execution, which makes the queueing pattern easy to reason about.

## Coroutine-style linking

`co_spawn` starts a coroutine on the `io_context`. The coroutine can wait asynchronously with `co_await`, then resume later when the timer expires. This gives you the familiar shape of sequential code while still being non-blocking.

In this example, a coroutine resumes, then schedules the next coroutine, which resumes, and so on:

```cpp
spawn_task("A", 30, [&]() {
    std::cout << "continuation from A: schedule B\n";
    spawn_task("B", 20, [&]() {
        std::cout << "continuation from B: schedule C and a plain post\n";
        spawn_task("C", 10, [] {
            std::cout << "continuation from C: final coroutine step\n";
        });
    });
});
```

This is the key idea: the async work is linked by the code flow, not by manually managing a bunch of callbacks.

## Why this comparison matters

Both APIs place work onto the same `io_context` and are ordered by the same event loop, but they encourage different mental models:

- `post` is best when you are enqueueing simple units of work
- `co_spawn` is best when your logic naturally reads as a sequence of asynchronous steps

The real difference is not “which one runs sooner,” but “how you express the relationship between steps.”

A queued callback chain is explicit and callback-driven. A coroutine chain is sequential and natural to read, even though it still yields to the same event loop underneath.

## Example run summary

The program output shows this ordering clearly:

```text
main: enqueue several post callbacks to show the queue
main: spawn multiple coroutines that link into each other
post #1: first item in the queue
post #4: another task that is queued independently
co_spawn A: start
post #2: scheduled from post #1, still in the same queue
post #3: final callback in the chain
co_spawn A: resume after wait
continuation from A: schedule B
co_spawn B: start
...
```

This demonstrates that both styles are simply different ways to enqueue and link asynchronous work on the same execution engine.

## See also

- API overview: [../../docs/boost_asio.md](../../docs/boost_asio.md)
- Design discussion: [../../docs/boost_asio_design.md](../../docs/boost_asio_design.md)
- Source: [post_vs_cospawn.cpp](post_vs_cospawn.cpp)
