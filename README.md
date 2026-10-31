# ITCH order book

A C++20 parser for Nasdaq TotalView-ITCH 5.0 and a per-stock limit order book
built from it, with a benchmark loop that replays a whole file and reports
nanoseconds per message. The measured progression of the book's data
structure, from 712 to 288 ns/msg, is in [results.md](results.md), together
with a re-run on named hardware.

## Layout

| Path | What it is |
|---|---|
| `ITCH_parser/parser_logic.{h,cpp}` | Reads message fields straight out of the byte buffer (big-endian, no copies), one struct per message type |
| `ITCH_parser/order_book.{h,cpp}` | The book: orders by reference number, price levels per stock in a sorted vector, stocks indexed by stock locate |
| `ITCH_parser/itch_parser.cpp` | The benchmark: 100 full-file replays, percentiles across replays |
| `ITCH_parser/byte_order.h` | Big-endian helpers so the code builds on Linux, macOS and Emscripten |
| `ITCH_parser/CMakeLists.txt`, `Dockerfile`, `run.sh` | Native build; `run.sh` builds and runs inside a throwaway Ubuntu container |
| `cost_of_operations/` | Submodule: microbenchmarks for memory access, branch prediction, false sharing and stride |
| `http_client/` | Submodule: a raw epoll HTTP and WebSocket server |
| `results.md` | Benchmark log |
| everything else at the top level | C++ learning scratch files |

## Build and run

Native, with [Google Benchmark](https://github.com/google/benchmark) installed
(only `DoNotOptimize` is used):

```sh
cmake -S ITCH_parser -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/itch_parser path/to/itch50.bin [stock_locate_to_snapshot]
```

Or in Docker, which builds Google Benchmark for you: `cd ITCH_parser && ./run.sh`.
With no arguments the program reads `sample_itch50.bin` from the working
directory and snapshots stock locate 393, as the benchmark log did.

## Input

No data file is included. The input is a TotalView-ITCH 5.0 capture in the
usual file form: each message is preceded by a 2-byte big-endian length.
Nasdaq publishes sample files of this format. The file the log was measured
on is a 50 MB prefix of such a capture (1,716,551 messages); it is not
redistributed here.

The message types applied to the book are A and F (add order), E and C
(executed), X (cancel), D (delete) and U (replace). R (stock directory)
messages at the start of the file size the per-stock arrays. Everything else
is skipped.

## Benchmark method, and how to read the numbers

- Each figure is the mean nanoseconds per message over one full-file replay,
  measured around the whole replay with `steady_clock`. The percentiles are
  across 100 such replays. They are not per-message latencies: "p99" is the
  99th slowest replay, not the slowest message.
- The timed loop also takes a 10-level book snapshot of one stock every 100
  messages, so that cost is inside the figure.
- The log in `results.md` was produced in a Linux container on hardware the
  log did not record. The `CMakeLists.txt` committed alongside those runs
  forced a Debug build, so treat the log as a relative progression. The
  re-run section in `results.md` names the CPU and uses a Release build.

## Known limits

- A fully executed order stays in the order map until the feed deletes it, so
  the map grows over a session.
- The book is single-threaded and reads a file; there is no network feed
  handler in this repository.
- Only the message types listed above are applied. Trade messages (P, Q) and
  administrative messages other than R are skipped.

A browser demo, orderbook.latencylabs.tech, compiles `parser_logic.cpp` and
`order_book.cpp` from this repository to WebAssembly and replays a generated
stream with fictional symbols.
