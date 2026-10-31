# ITCH Order Book — Benchmark Results

Latency is per-message processing time (parse + book update), in nanoseconds.

## How to read these numbers

- Each figure is the mean ns/msg over one full replay of a 1,716,551-message
  sample file (`itch_parser.cpp`, main loop). The percentile is across 100
  such replays, so "p99" is the 99th slowest replay, not a per-message tail.
- The timed loop includes a 10-level book snapshot of one stock every 100
  messages.
- These runs were made in a Linux container (`run.sh`) on hardware this log did
  not record, and the committed `CMakeLists.txt` forced a Debug build at each
  of these commits. Read them as a relative progression; the re-run at the end
  names the hardware and uses a Release build.
- The book code at these commits had defects found later (executions added
  shares to a level instead of removing them, the snapshot held no asks). They
  were fixed before the re-run, so the re-run is not directly comparable with
  the log either.

## Map-based order book (stock string as key)

Order book stored in `std::map`/hash structures keyed by the stock symbol string; each price level looked up via the symbol key. Simple and correct, but the string-keyed lookups and tree/hash traversal on every message put a floor on throughput.

| Percentile | Latency (ns/msg) |
|------------|------------------|
| 50th       | 712              |
| 99th       | 754              |
| 99.9th     | 754              |

## Map-based order book (uint16_t stock locate as key)

Same map-based structure, but keyed by the 2-byte `uint16_t` stock locate instead of the stock symbol string. Swapping string hashing/comparison for a cheap integer key roughly halves the median latency.

| Percentile | Latency (ns/msg) |
|------------|------------------|
| 50th       | 523              |
| 99th       | 641              |
| 99.9th     | 641              |

> Tail note: an earlier run of this same build showed p99/p99.9 of 941; a re-run with an unchanged median gave 641. The tail moving while the median stays flat (and p99 == p99.9 over only 100 samples) points to context-switch / scheduler noise on one sample, not algorithmic cost.

## Vector-based order book (uint16_t stock locate as key)

Per-stock price levels held in a price-sorted `std::vector<OrderBookNode>` (keyed by `uint16_t` stock locate) instead of an inner map. The contiguous, cache-friendly layout and cheaper traversal drop both the median and the tail again, and the tail is now tight (p99 within ~3% of p50).

| Percentile | Latency (ns/msg) |
|------------|------------------|
| 50th       | 483              |
| 99th       | 498              |
| 99.9th     | 498              |

## Vector-based order book, no per-order symbol copy

Same vector layout, but the `std::string sym` copy is dropped from the per-order path (the symbol is no longer stored/copied on every add). Removing that per-message string construction shaves the median further.

| Percentile | Latency (ns/msg) |
|------------|------------------|
| 50th       | 413              |
| 99th       | —                |
| 99.9th     | —                |

## Flat vector outer storage (stock locate as direct array index)

Outer `std::map<uint16_t, ...>` replaced with a flat `std::vector<std::vector<OrderBookNode>>` indexed directly by stock locate. The per-message per-stock lookup goes from an O(log n) red-black-tree pointer chase to a single O(1) array index, eliminating the tree's cache misses — the biggest single drop so far.

| Percentile | Latency (ns/msg) |
|------------|------------------|
| 50th       | 288              |
| 99th       | 331              |
| 99.9th     | 331              |

## Binary-search insertion in add_to_book (std::lower_bound)

Price-level insertion point found with `std::lower_bound` (O(log n) binary search) instead of a linear scan. The median is unchanged (288) — at typical top-of-book depth the level vectors are short enough that the linear scan was never the bottleneck, so O(log n) search doesn't move the needle. Worth keeping for correctness/scaling on deep books, but not a latency win here.

| Percentile | Latency (ns/msg) |
|------------|------------------|
| 50th       | 288              |
| 99th       | 338              |
| 99.9th     | 338              |

## Re-run on named hardware (2026-10-31)

Code at commit `a87ddbe` (book fixes, portable byte order, Release build).
Same method: 100 full-file replays, mean ns/msg per replay, percentiles across
replays, 10-level snapshot every 100 messages. Three runs of the program each.

- CPU: Apple M3 Pro (11 cores), macOS 14.8.9, Apple clang 15.0.0
- Build: `cmake -DCMAKE_BUILD_TYPE=Release` (`-O3 -DNDEBUG`), Google Benchmark 1.9.5 for `DoNotOptimize`
- Nothing else running on the machine was controlled for; this is a laptop, not a pinned core.

### Sample file, 1,716,551 messages per replay, snapshot of stock locate 393

| Run | p50 (ns/msg) | p99 (ns/msg) |
|-----|--------------|--------------|
| 1   | 63.95        | 108.47       |
| 2   | 63.11        | 66.91        |
| 3   | 62.42        | 72.24        |

### Generated stream, 100,000 messages per replay, snapshot of stock locate 1

A seeded synthetic ITCH 5.0 stream with 12 fictional symbols (seed
`orderbook-demo-1`), the same stream the browser demo replays.

| Run | p50 (ns/msg) | p99 (ns/msg) |
|-----|--------------|--------------|
| 1   | 68.39        | 110.71       |
| 2   | 69.66        | 176.04       |
| 3   | 69.22        | 79.21        |

With only 100 samples per run, p99 is the second-slowest replay and moves
between runs; p50 is the number to compare.
