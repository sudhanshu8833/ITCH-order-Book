# ITCH-order-Book

A NASDAQ TotalView-ITCH 5.0 parser and limit order book in C++20.

It memory-maps a raw ITCH 5.0 feed, decodes the binary messages, rebuilds a
per-stock order book, and reports the parser's per-message latency.

## What it does

- Decodes the book-building ITCH 5.0 messages: Add Order (`A`/`F`), Order
  Executed (`E`), Order Executed With Price (`C`), Order Cancel (`X`), Order
  Delete (`D`) and Order Replace (`U`).
- Tracks each order's lifecycle (added -> partially executed -> executed /
  cancelled) in an `OrderBook` keyed by stock-locate code, using flat
  `unordered_map` / `map` containers and fixed-width integer fields.
- Reads the feed with `mmap` and big-endian field decoding, walking the mapped
  bytes directly with no per-message heap allocation on the hot path.

## Benchmark, honestly

The program times a full parse over 100 passes with `std::chrono::steady_clock`
and prints the p50, p99 and p99.9 nanoseconds-per-message. Google Benchmark's
`DoNotOptimize` keeps the compiler from eliding the work. These numbers measure
decode-and-book-update cost on one machine over one sample file; they are not a
claim about end-to-end exchange latency, the network path, or any other feed.

## Build and run

The build runs in Docker so the Google Benchmark dependency is pinned:

    cd ITCH_parser
    ./run.sh          # builds the image if needed, then compiles and runs
    ./run.sh -b       # force an image rebuild

It expects a raw ITCH 5.0 file named `sample_itch50.bin` in the working
directory. Requires C++20 and CMake 3.14+.

## Layout

- `ITCH_parser/parser_logic.*`  - message decoders and big-endian field reads
- `ITCH_parser/order_book.*`    - the order book and order lifecycle
- `ITCH_parser/itch_parser.cpp` - entry point, feed mmap and timing loop
