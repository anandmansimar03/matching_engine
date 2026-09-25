# Matching Engine

A low-latency **C++23 limit order book and matching engine** supporting multiple symbols, limit and market orders, price-time priority, order cancellation, and trade generation.

The project is designed as a compact, modular implementation of the core components found in an electronic trading system.

## Features

* C++23 implementation
* Multiple instrument/symbol support
* Limit orders
* Market orders
* Order cancellation
* Price-time priority
* Best bid / best ask lookup
* Quantity lookup at a price level
* Trade generation
* Partial fills
* Multiple fills for a single incoming order
* Separate order books per symbol
* Pre-allocated order storage
* Modular order-book architecture
* CMake build system

---

## Architecture

The matching engine is organized into three primary layers:

```text
                     +-------------------+
                     |   GlobalOrderBook |
                     +---------+---------+
                               |
              +----------------+----------------+
              |                                 |
          AAPL OrderBook                    MSFT OrderBook
              |                                 |
       +------+-------+                 +-------+------+
       |              |                 |              |
     Bids            Asks             Bids            Asks
       |              |                 |              |
   Price Levels   Price Levels      Price Levels   Price Levels
       |              |                 |              |
     Orders         Orders            Orders         Orders
```

### GlobalOrderBook

`GlobalOrderBook` manages order books for different symbols.

Responsibilities:

* Create and access order books
* Route orders to the appropriate symbol
* Handle symbol-level operations
* Maintain the global order pool

### OrderBook

`OrderBook` represents the order book for a single instrument.

Responsibilities:

* Maintain bids and asks
* Match incoming orders
* Maintain price-time priority
* Handle cancellations
* Generate trades
* Provide best-price and quantity queries

### Order

An `Order` contains the information required to process an order:

```cpp
Order {
    order_id
    side
    order_type
    price
    qty
    original_qty
}
```

---

## Matching Algorithm

The engine follows standard **price-time priority**.

### Buy orders

For a buy order:

1. Match against the lowest available ask price.
2. At the same price, match the oldest order first.
3. Continue until:

   * the incoming order is completely filled, or
   * no executable ask remains.

### Sell orders

For a sell order:

1. Match against the highest available bid price.
2. At the same price, match the oldest order first.
3. Continue until:

   * the incoming order is completely filled, or
   * no executable bid remains.

### Example

Suppose the ask side contains:

```text
ASK

Price      Quantity
102.00        200
101.00        100
```

An incoming:

```text
BUY 150 @ 102.00
```

will execute as:

```text
100 @ 101.00
 50 @ 102.00
```

The remaining ask book becomes:

```text
ASK

Price      Quantity
102.00        150
```

---

## Price-Time Priority

Orders at better prices always have priority.

If multiple orders exist at the same price, the earliest order receives priority.

Example:

```text
SELL 100 @ 103.00   Order 4001
SELL  75 @ 103.00   Order 4002
```

An incoming:

```text
BUY 60 @ 103.00
```

matches:

```text
Order 4001
60 @ 103.00
```

leaving:

```text
Order 4001 -> 40 remaining
Order 4002 -> 75 remaining
```

---

## Supported Order Types

### Limit Order

A limit order specifies the maximum price a buyer is willing to pay or the minimum price a seller is willing to accept.

Example:

```text
BUY 100 @ 101.00
```

The order can execute at `101.00` or better.

If it cannot be immediately matched, the remaining quantity rests on the order book.

### Market Order

A market order does not specify a price.

It consumes available liquidity starting from the best available price.

Example:

```text
BUY 100 MARKET
```

will consume the lowest available asks until either:

* 100 units have traded, or
* the ask side becomes empty.

---

## Order Cancellation

Existing orders can be cancelled using their order ID.

Example:

```cpp
book.cancel_order("AAPL", 2001);
```

Cancellation removes the remaining quantity from the order book.

Already executed quantity cannot be cancelled.

---

## Trade Generation

Every successful match generates a trade containing information such as:

```text
TRADE
symbol=AAPL
seq=...
price=10100
qty=100
buyer=3001
seller=1001
```

A single incoming order can generate multiple trades.

For example:

```text
BUY 250 @ 103.00
```

may produce:

```text
100 @ 101.00
100 @ 102.00
 50 @ 103.00
```

---

## Project Structure

```text
matching_engine/
├── CMakeLists.txt
├── README.md
│
├── src/
│   ├── main.cpp
│   │
│   ├── include/
│   │   ├── global_order_book.hpp
│   │   ├── order_book.hpp
│   │   ├── order.hpp
│   │   ├── types.hpp
│   │   └── ...
│   │
│   └── ...
│
└── build/
```

The implementation separates the public interfaces and data structures from the executable entry point.

---

## Requirements

### Compiler

A compiler supporting C++23 is required.

Recommended:

* Clang 16+
* GCC 13+
* Apple Clang with C++23 support

### Build System

CMake 3.20+ is recommended.

---

## Building

Clone the repository:

```bash
git clone https://github.com/anandmansimar03/matching_engine.git
cd matching_engine
```

Create a build directory:

```bash
cmake -S . -B build
```

Build:

```bash
cmake --build build -j
```

Run:

```bash
./build/order_book_demo
```

For a clean build:

```bash
rm -rf build
cmake -S . -B build
cmake --build build -j
```

---

## Example

The demo in `main.cpp` creates an AAPL order book.

Initial asks:

```text
ASK
101.00 x 100
102.00 x 200
```

Initial bids:

```text
BID
99.00 x 150
98.00 x 100
```

An aggressive buy order is then submitted:

```text
BUY 150 @ 102.00
```

The matching engine generates:

```text
TRADE price=101.00 qty=100
TRADE price=102.00 qty=50
```

The remaining ask quantity is:

```text
102.00 x 150
```

---

## Important Design Choices

### Integer Prices

Prices are represented using integer types rather than floating-point values.

For example:

```text
101.00 -> 10100
102.50 -> 10250
```

This avoids floating-point precision problems when comparing prices.

The exact price scaling factor can be defined by the application using the engine.

### Integer Quantities

Order quantities are also represented using integer types.

This avoids fractional rounding problems during matching.

### Price-Level Organization

Orders are grouped by price level.

Conceptually:

```text
Price Level
    |
    +-- Order 1
    +-- Order 2
    +-- Order 3
```

Orders within a price level maintain time priority.

### Separate Bid and Ask Books

The engine maintains independent bid and ask sides.

```text
        ORDER BOOK

        ASK
        103.00
        102.00
        101.00
        --------
        100.00
         99.00
         98.00
        BID
```

For bids, higher prices have priority.

For asks, lower prices have priority.

---

## Complexity

The exact complexity depends on the underlying containers used by the implementation, but the target design is based around efficient price-level access and constant-time order removal where possible.

Conceptually:

| Operation         |                      Typical complexity |
| ----------------- | --------------------------------------: |
| Add order         |                                O(log P) |
| Find best bid/ask | O(1) / O(log P), depending on structure |
| Match             |                      O(number of fills) |
| Cancel            |     O(log P) or O(1) after order lookup |
| Quantity at price | O(log P) / O(1), depending on structure |

Where `P` is the number of active price levels.

The matching operation itself is naturally proportional to the number of orders consumed.

---

## Sequence Numbers

Trade and cancellation events can be associated with sequence numbers.

This provides a deterministic ordering of generated events and is useful for:

* market-data generation
* replay
* backtesting
* reconciliation
* downstream consumers

---

## Determinism

Given the same initial order book and the same sequence of incoming orders, the engine should produce the same sequence of matches and resulting book state.

This is an important property for:

* unit testing
* historical replay
* backtesting
* debugging
* market-data validation

---

## Potential Extensions

The current engine provides the core matching functionality. Possible future improvements include:

### 1. Unit Tests

Add tests covering:

* Basic matching
* Partial fills
* Multiple fills
* Price-time priority
* Market orders
* Cancellation
* Empty books
* Multiple symbols
* Self-trade scenarios
* Invalid orders

### 2. Command-Line Input

Support an input stream such as:

```text
ADD AAPL 1001 BUY LIMIT 10000 100
ADD AAPL 1002 SELL LIMIT 10100 50
ADD AAPL 1003 BUY LIMIT 10100 75
CANCEL AAPL 1001
```

### 3. Market Data Output

Generate:

```text
ADD
CANCEL
TRADE
BOOK_UPDATE
```

events suitable for downstream consumers.

### 4. Order IDs and Lookup

Maintain a direct mapping:

```text
order_id -> order
```

to make cancellation efficient.

### 5. Performance Benchmarking

Add benchmarks for:

* orders/sec
* matches/sec
* cancellation throughput
* p50 latency
* p99 latency
* p99.9 latency
* memory usage

### 6. Multi-Threaded Architecture

A production-oriented architecture could separate:

```text
Market Data
     |
     v
Order Gateway
     |
     v
Sequencer
     |
     v
Matching Engine
     |
     +----> Trade Publisher
     |
     +----> Market Data Publisher
```

The actual matching path should generally remain single-threaded per instrument/order book to preserve deterministic ordering and avoid unnecessary synchronization.

---

## Performance Considerations

This project is intended as a learning and systems-programming implementation rather than a production exchange engine.

For a latency-sensitive production implementation, additional considerations would include:

* Cache locality
* Memory allocation
* Object lifetime
* NUMA placement
* CPU affinity
* branch prediction
* data structure selection
* lock avoidance
* false sharing
* batching
* kernel/network configuration
* timestamping
* hardware topology

A useful performance target is not simply maximum throughput. For a trading system, **deterministic latency** and tail latency are often equally important.

---

## Testing Strategy

A robust test suite should validate both individual operations and complete book states.

Example:

```text
Initial:
    Ask: 10100 x 100

Incoming:
    Buy: 10100 x 60

Expected:
    Trade: 10100 x 60

Remaining:
    Ask: 10100 x 40
```

Another example:

```text
Ask:
    10100 x 100
    10200 x 200

Incoming:
    Buy 150 @ 10200

Expected:
    10100 x 100
    10200 x 50
```

Cancellation:

```text
Initial:
    Bid 9900 x 100

Cancel:
    order_id = 123

Expected:
    Bid side is empty
```

---

## Disclaimer

This project is an educational implementation of a matching engine.

It is **not intended for production trading or use as an exchange matching engine without substantial additional development, testing, validation, and operational controls.**

---

## License

Add the project's license here if/when one is selected.

---

## Author

**Mansimar Anand**

C++ / Market Data / Low-Latency Systems

GitHub:

https://github.com/anandmansimar03

```
