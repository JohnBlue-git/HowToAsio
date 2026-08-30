# Boost.Asio Design and Execution Model

The core design question in Asio is not just “how do I schedule a callback,” but “which execution model matches the service and which objects need serialized access?”

## 1. Design overview

The major choices are:

- one shared `io_context`
- multiple worker threads driving the same context
- one or more `strand` objects for shared state
- callback or coroutine flow for async logic

A useful text graph is:

```text
client / timer / socket / D-Bus event
                │
                ▼
         async operation
                │
                ▼
         io_context queue
                │
                ▼
          handler or coroutine
                │
                ▼
        shared state / response
```

## 2. Comparison table: architecture choices

| Design | Concurrency | State safety | Typical use | Main drawback |
| --- | --- | --- | --- | --- |
| Single `io_context` | Low to moderate | Depends on object ownership | Small or medium services | Busy loop becomes bottleneck |
| Multi-threaded `io_context` | High | Needs careful state boundaries | I/O-heavy services | Races if shared state is unmanaged |
| `strand` per object | Controlled concurrency | Strong | session/connection/stateful object | Can reduce throughput |
| Mixed structure: one context + multiple strands | Balanced | Good when boundaries are clear | real service code | More architecture decisions |

## 3. Single `io_context` model

```cpp
boost::asio::io_context io;
```

This is the simplest design. A single scheduler runs the whole service. It is easy to reason about and easy to implement for a small server.

### When it works well

- small or medium service
- low to moderate request volume
- logically centralized event loop

### Trade-off

A single busy `io_context` may become the bottleneck if all work is competing in the same queue.

## 4. Multi-threaded `io_context` model

```cpp
boost::asio::io_context io;

std::thread t1([&] { io.run(); });
std::thread t2([&] { io.run(); });
```

This design improves throughput when work is naturally independent.

### When it works well

- I/O-heavy workloads
- handlers mostly do not touch the same mutable objects
- you intentionally separate independent subsystems

### Trade-off

The same object can be touched by multiple handlers concurrently if no state boundary exists. This is where strands and ownership matter.

## 5. `strand` as a serialization boundary

```cpp
auto strand = boost::asio::make_strand(io);
```

A `strand` serializes callbacks from the same strand and is a standard tool for protecting per-object state.

### Typical pattern

- one strand per connection or session
- handlers for that connection bind to the strand
- shared state remains ordered without a manual mutex for every callback

### When not to overuse it

A single global strand is safe but frequently reduces concurrency more than needed.

## 6. `post` vs `dispatch` in design terms

| API | Behavior | Best use |
| --- | --- | --- |
| `post` | Always queues work | explicit deferral, cross-context handoff |
| `dispatch` | Runs inline when already in correct context | current-context work, no queue hop |

`post` is the safer default. `dispatch` is the optimization when the work already belongs to the current executor.

## 7. `post` vs `co_spawn`

| API | Pattern | Best fit |
| --- | --- | --- |
| `post` | callback style | small tasks, fire-and-forget work |
| `co_spawn` | coroutine style | long async flow with several waits |

Coroutines are often clearer when the async logic naturally reads as a sequence.

## 8. Realistic service architecture

```text
main thread
  ├── create io_context
  ├── start worker threads
  ├── create major strands
  └── register async operations

worker threads
  └── io.run()

subsystems
  ├── HTTP handlers
  ├── D-Bus work
  ├── timers
  └── background tasks
```

This is the practical form that many server services use: one scheduler, explicit subsystem boundaries, and ordered access at the stateful edges.

The design discussion here complements the API overview in [boost_asio.md](boost_asio.md). The examples in [examples/io_context_vs_strand/io_context_vs_strand.md](examples/io_context_vs_strand/io_context_vs_strand.md), [examples/post_vs_dispatch/post_vs_dispatch.md](examples/post_vs_dispatch/post_vs_dispatch.md), and [examples/post_vs_cospawn/post_vs_cospawn.md](examples/post_vs_cospawn/post_vs_cospawn.md) show the same ideas in concrete code.

## 9. Common mistakes

### Mistake 1: believing `io_context` guarantees thread safety
It does not. It only schedules work.

### Mistake 2: using one giant global strand
This is correct but often over-serializes the system.

### Mistake 3: mixing unrelated resources under one callback path
This makes the code hard to understand and harder to debug.

### Mistake 4: ignoring object ownership
Async state is usually safe only when ownership and execution boundaries are clear.

## 10. Recommended design rule

- independent objects stay independent
- shared state gets a strand or explicit serialization
- sequential async flow uses coroutines
- short fire-and-forget work uses `post`
- work already in the right context uses `dispatch`

This is the heart of effective Asio architecture.

## 11. See also

- API fundamentals: [boost_asio.md](boost_asio.md)
- Concrete comparisons: [examples/io_context_vs_strand/io_context_vs_strand.md](examples/io_context_vs_strand/io_context_vs_strand.md), [examples/post_vs_dispatch/post_vs_dispatch.md](examples/post_vs_dispatch/post_vs_dispatch.md), [examples/post_vs_cospawn/post_vs_cospawn.md](examples/post_vs_cospawn/post_vs_cospawn.md)
- Real service case: [openbmc_bmcweb.md](openbmc_bmcweb.md)
