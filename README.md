# HowToBoostAsio

This repository is a focused study project for Boost.Asio. It is organized around a single question: how do asynchronous services actually schedule work, serialize state, and coordinate multiple subsystems without turning into a race-prone callback mess?

## What this repo covers

- the core Asio execution model
- the most important API families and scheduling primitives
- design-level choices such as a single `io_context`, multi-threaded execution, and strands
- comparison examples for real scheduling patterns
- a real-world architectural note based on OpenBMC bmcweb

## Start here

1. [docs/boost_asio.md](docs/boost_asio.md) — fundamentals and API families
2. [docs/boost_asio_design.md](docs/boost_asio_design.md) — architecture and trade-offs
3. [examples](examples) — concrete comparison programs and short write-ups
4. [docs/openbmc_bmcweb.md](docs/openbmc_bmcweb.md) — real service pattern in bmcweb

## Document map

- [docs/boost_asio.md](docs/boost_asio.md): introduction to Boost.Asio and the core API categories
- [docs/boost_asio_design.md](docs/boost_asio_design.md): architecture choices and comparison tables
- [docs/openbmc_bmcweb.md](docs/openbmc_bmcweb.md): real bmcweb implementation patterns and design interpretation
- [examples/io_context_vs_strand/io_context_vs_strand.md](examples/io_context_vs_strand/io_context_vs_strand.md): `io_context` vs `strand`
- [examples/post_vs_dispatch/post_vs_dispatch.md](examples/post_vs_dispatch/post_vs_dispatch.md): `post` vs `dispatch`
- [examples/post_vs_cospawn/post_vs_cospawn.md](examples/post_vs_cospawn/post_vs_cospawn.md): `post` vs `co_spawn`

## Project structure

```text
HowToBoostAsio/
├── README.md
├── .gitignore
├── docs/
│   ├── boost_asio.md
│   ├── boost_asio_design.md
│   └── openbmc_bmcweb.md
├── examples/
│   ├── CMakeLists.txt
│   ├── io_context_vs_strand/
│   │   ├── io_context_vs_strand.cpp
│   │   └── io_context_vs_strand.md
│   ├── post_vs_dispatch/
│   │   ├── post_vs_dispatch.cpp
│   │   └── post_vs_dispatch.md
│   └── post_vs_cospawn/
│       ├── post_vs_cospawn.cpp
│       └── post_vs_cospawn.md
└── build/
```

## Build

```bash
cmake -S examples -B build
cmake --build build -j
```

The project uses CMake and `FetchContent` so Boost is fetched automatically during configuration.

## Run the comparison examples

```bash
./build/io_context_vs_strand
./build/post_vs_dispatch
./build/post_vs_cospawn
```

## Reading order

1. [docs/boost_asio.md](docs/boost_asio.md)
2. [docs/boost_asio_design.md](docs/boost_asio_design.md)
3. [examples/io_context_vs_strand/io_context_vs_strand.md](examples/io_context_vs_strand/io_context_vs_strand.md)
4. [examples/post_vs_dispatch/post_vs_dispatch.md](examples/post_vs_dispatch/post_vs_dispatch.md)
5. [examples/post_vs_cospawn/post_vs_cospawn.md](examples/post_vs_cospawn/post_vs_cospawn.md)
6. [docs/openbmc_bmcweb.md](docs/openbmc_bmcweb.md)

## Design theme

The repo intentionally focuses on the comparison between:

- single-threaded vs multi-threaded event loops
- plain `io_context` vs `strand`
- `post` vs `dispatch`
- callback-based work vs coroutine-based work

This is the practical mental model behind many Asio-based server systems.
