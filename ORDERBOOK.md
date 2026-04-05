# Central Limit Order Book (CLOB) Implementation in C++

A high-performance implementation of a Central Limit Order Book matching engine in modern C++.

## Overview

This is a fully functional Central Limit Order Book (CLOB) implementation that demonstrates the core functionality of electronic trading systems. The order book maintains buy and sell orders, matches them based on price-time priority, and executes trades.

## Features

### Core Functionality
- **Price-Time Priority Matching**: Orders are matched based on best price first, then time priority (FIFO)
- **Order Types**: Support for both Limit and Market orders
- **Order Operations**:
  - Add orders (with automatic matching)
  - Cancel orders
  - Modify orders (cancel and replace)
- **Real-time Trade Execution**: Immediate matching and trade generation
- **Trade Callbacks**: Asynchronous notification of executed trades

### Data Structures
- **Efficient Price Levels**: Uses `std::map` with custom comparators for O(log n) best bid/ask lookup
- **Fast Order Lookup**: Hash map for O(1) order retrieval by ID
- **FIFO Queue**: `std::deque` for maintaining time priority within price levels

## Architecture

### Components

1. **Order (`order.hpp`)**
   - Basic order structure with ID, symbol, side, type, price, and quantity
   - Support for partial fills
   - Timestamp for time priority

2. **Trade (`order.hpp`)**
   - Represents executed trades
   - Contains matched buy/sell order IDs, price, and quantity

3. **PriceLevel (`order_book.hpp`)**
   - Groups all orders at a specific price
   - Maintains total quantity at the level
   - FIFO queue for time priority

4. **OrderBook (`order_book.hpp`, `order_book.cpp`)**
   - Main matching engine
   - Separate buy (bids) and sell (asks) sides
   - Matching logic and order management

## Usage Example

```cpp
#include "order.hpp"
#include "order_book.hpp"
#include <chrono>
#include <iostream>
#include <memory>

int main() {
  // Create an order book for a symbol
  OrderBook book("AAPL");

  // Set up trade callback
  book.set_trade_callback([](const Trade &trade) {
    std::cout << "TRADE: Price=" << trade.price
              << " Quantity=" << trade.quantity << "\n";
  });

  auto now = std::chrono::system_clock::now().time_since_epoch().count();

  // Add a buy order (bid)
  auto buy_order = std::make_shared<Order>(
      1, "AAPL", Side::BUY, OrderType::LIMIT, 150.00, 100, now++);
  book.add_order(buy_order);

  // Add a sell order (ask)
  auto sell_order = std::make_shared<Order>(
      2, "AAPL", Side::SELL, OrderType::LIMIT, 151.00, 100, now++);
  book.add_order(sell_order);

  // Print the order book
  book.print();

  // Get market data
  auto best_bid = book.get_best_bid();
  auto best_ask = book.get_best_ask();
  auto spread = book.get_spread();

  // Add an aggressive order that matches
  auto aggressive_buy = std::make_shared<Order>(
      3, "AAPL", Side::BUY, OrderType::LIMIT, 151.00, 50, now++);
  book.add_order(aggressive_buy); // This will execute a trade

  return 0;
}
```

## Matching Logic

### Price-Time Priority
1. **Price Priority**: Best price gets matched first
   - For buy orders: highest price first
   - For sell orders: lowest price first

2. **Time Priority**: Within the same price level, earlier orders get matched first (FIFO)

### Matching Process
1. Incoming order is compared against the opposite side of the book
2. If prices cross (buy price ≥ sell price), a trade occurs
3. Trade executes at the **resting order's price** (maker price)
4. Quantities are reduced accordingly
5. Fully filled orders are removed from the book
6. Partially filled orders remain in the book
7. Unfilled portion of incoming order is added to the book (for limit orders)

### Order Types
- **Limit Orders**: Execute at specified price or better, remainder goes into the book
- **Market Orders**: Execute immediately at best available price, unfilled portion is rejected

## API Reference

### OrderBook Methods

#### `add_order(std::shared_ptr<Order> order)`
Adds a new order to the book and attempts to match it. Returns a vector of executed trades.

#### `cancel_order(uint64_t order_id)`
Cancels an existing order. Returns `true` if successful.

#### `modify_order(uint64_t order_id, double new_price, uint64_t new_quantity)`
Modifies an existing order (cancel and replace). Returns `true` if successful.

#### `get_best_bid()`
Returns the highest buy price in the book, or `std::nullopt` if no bids exist.

#### `get_best_ask()`
Returns the lowest sell price in the book, or `std::nullopt` if no asks exist.

#### `get_spread()`
Returns the difference between best ask and best bid, or `std::nullopt` if either side is empty.

#### `get_bid_volume_at_price(double price)`
Returns total quantity of buy orders at a specific price level.

#### `get_ask_volume_at_price(double price)`
Returns total quantity of sell orders at a specific price level.

#### `set_trade_callback(TradeCallback callback)`
Sets a callback function to be invoked when trades are executed.

#### `print()`
Prints a formatted view of the current order book state.

## Building

The project uses CMake:

```bash
# Configure
cmake --preset=debug

# Build
cmake --build build/debug

# Run
./build/debug/src/myproject
```

## Performance Characteristics

- **Add Order**: O(log n) for price level lookup + O(1) for adding to queue
- **Cancel Order**: O(1) for order lookup + O(n) for removal from price level queue
- **Modify Order**: Same as cancel + add
- **Get Best Bid/Ask**: O(1) (first element of sorted map)
- **Matching**: O(m) where m is the number of orders matched

## Design Decisions

1. **std::map with custom comparators**: Provides automatic sorting of price levels
   - Bids: `std::greater<double>` for descending order (highest first)
   - Asks: `std::less<double>` for ascending order (lowest first)

2. **std::deque for order queues**: Efficient front removal (O(1)) for FIFO processing

3. **std::unordered_map for order lookup**: Fast O(1) access to orders by ID

4. **Smart pointers**: Automatic memory management and safe sharing of order objects

5. **Callback pattern**: Decoupled trade notification for flexibility

## Future Enhancements

- [ ] Stop orders and other advanced order types
- [ ] Order book depth/market data snapshots
- [ ] Iceberg orders (hidden quantity)
- [ ] Time-in-force conditions (GTC, IOC, FOK)
- [ ] Multi-threaded support with lock-free data structures
- [ ] Market data feed generation
- [ ] Order book persistence
- [ ] Performance benchmarking and optimization

## License

This is an educational implementation for learning purposes.

