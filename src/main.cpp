#include "include/global_order_book.hpp"

#include <cstdint>
#include <iostream>
#include <string>

namespace {

using namespace types;
using namespace order_book;

void print_result(const std::string& symbol, const MatchResult& result)
{
    for (const auto& trade : result.trades) {
        std::cout
            << "TRADE"
            << " symbol=" << symbol
            << " seq=" << trade.seq_num
            << " price=" << trade.price
            << " qty=" << trade.qty
            << " buyer=" << trade.buyer_id
            << " seller=" << trade.seller_id
            << '\n';
    }

    for (const auto& cancel : result.cancels) {
        std::cout
            << "CANCEL"
            << " symbol=" << symbol
            << " seq=" << cancel.seq_num
            << " order_id=" << cancel.order_id
            << " qty=" << cancel.qty
            << " original_qty=" << cancel.original_qty
            << " reason=" << static_cast<int>(cancel.cancel_reason)
            << '\n';
    }
}

Order make_limit_order(
    const OrderIdT order_id,
    const SideEnum side,
    const PriceT price,
    const QtyT qty)
{
    return Order{
        .order_id = order_id,
        .side = side,
        .order_type = OrderTypeEnum::Limit,
        .price = price,
        .qty = qty,
        .original_qty = 0,
    };
}

Order make_market_order(
    const OrderIdT order_id,
    const SideEnum side,
    const QtyT qty)
{
    return Order{
        .order_id = order_id,
        .side = side,
        .order_type = OrderTypeEnum::Market,
        .price = 0,
        .qty = qty,
        .original_qty = 0,
    };
}

} // namespace

int main()
{
    constexpr std::size_t ORDER_POOL_CAPACITY = 1024;

    GlobalOrderBook book{ORDER_POOL_CAPACITY};

    constexpr const char* SYMBOL = "AAPL";

    std::cout << "=== Matching Engine Demo ===\n\n";

    // ------------------------------------------------------------
    // 1. Add resting sell orders
    // ------------------------------------------------------------

    std::cout << "--- Adding asks ---\n";

    auto result = book.add_order(
        SYMBOL,
        make_limit_order(
            1001,
            SideEnum::Ask,
            10100,
            100));

    print_result(SYMBOL, result);

    result = book.add_order(
        SYMBOL,
        make_limit_order(
            1002,
            SideEnum::Ask,
            10200,
            200));

    print_result(SYMBOL, result);

    // ------------------------------------------------------------
    // 2. Add resting buy orders
    // ------------------------------------------------------------

    std::cout << "\n--- Adding bids ---\n";

    result = book.add_order(
        SYMBOL,
        make_limit_order(
            2001,
            SideEnum::Bid,
            9900,
            150));

    print_result(SYMBOL, result);

    result = book.add_order(
        SYMBOL,
        make_limit_order(
            2002,
            SideEnum::Bid,
            9800,
            100));

    print_result(SYMBOL, result);

    // ------------------------------------------------------------
    // 3. Display best bid / ask
    // ------------------------------------------------------------

    std::cout << "\n--- Book ---\n";

    std::cout << "Best bid: "
              << book.best_bid(SYMBOL)
              << '\n';

    std::cout << "Best ask: "
              << book.best_ask(SYMBOL)
              << '\n';

    std::cout << "Bid quantity @ 9900: "
              << book.qty_at(
                     SYMBOL,
                     SideEnum::Bid,
                     9900)
              << '\n';

    std::cout << "Ask quantity @ 10100: "
              << book.qty_at(
                     SYMBOL,
                     SideEnum::Ask,
                     10100)
              << '\n';

    // ------------------------------------------------------------
    // 4. Aggressive limit BUY
    //
    // Existing asks:
    //
    // 1001: SELL 100 @ 10100
    // 1002: SELL 200 @ 10200
    //
    // Incoming:
    //
    // 3001: BUY 150 @ 10200
    //
    // Expected:
    //
    // 100 @ 10100 against 1001
    // 50  @ 10200 against 1002
    // ------------------------------------------------------------

    std::cout << "\n--- Aggressive BUY ---\n";

    result = book.add_order(
        SYMBOL,
        make_limit_order(
            3001,
            SideEnum::Bid,
            10200,
            150));

    print_result(SYMBOL, result);

    // ------------------------------------------------------------
    // 5. Check book after matching
    // ------------------------------------------------------------

    std::cout << "\n--- Book after trade ---\n";

    std::cout << "Best bid: "
              << book.best_bid(SYMBOL)
              << '\n';

    std::cout << "Best ask: "
              << book.best_ask(SYMBOL)
              << '\n';

    std::cout << "Ask quantity @ 10200: "
              << book.qty_at(
                     SYMBOL,
                     SideEnum::Ask,
                     10200)
              << '\n';

    // ------------------------------------------------------------
    // 6. Demonstrate price-time priority
    //
    // Add two asks at the same price.
    // Order 4001 arrives before 4002, so 4001 must execute first.
    // ------------------------------------------------------------

    std::cout << "\n--- Price-time priority ---\n";

    result = book.add_order(
        SYMBOL,
        make_limit_order(
            4001,
            SideEnum::Ask,
            10300,
            50));

    print_result(SYMBOL, result);

    result = book.add_order(
        SYMBOL,
        make_limit_order(
            4002,
            SideEnum::Ask,
            10300,
            75));

    print_result(SYMBOL, result);

    // Incoming BUY should consume order 4001 first.
    result = book.add_order(
        SYMBOL,
        make_limit_order(
            3002,
            SideEnum::Bid,
            10300,
            60));

    print_result(SYMBOL, result);

    // ------------------------------------------------------------
    // 7. Demonstrate cancellation
    // ------------------------------------------------------------

    std::cout << "\n--- Cancellation ---\n";

    result = book.cancel_order(
        SYMBOL,
        2001);

    print_result(SYMBOL, result);

    // ------------------------------------------------------------
    // 8. Demonstrate market order
    //
    // Remaining asks will be consumed until the market order
    // is completely filled or liquidity runs out.
    // ------------------------------------------------------------

    std::cout << "\n--- Market BUY ---\n";

    result = book.add_order(
        SYMBOL,
        make_market_order(
            5001,
            SideEnum::Bid,
            100));

    print_result(SYMBOL, result);

    // ------------------------------------------------------------
    // 9. Final book state
    // ------------------------------------------------------------

    std::cout << "\n--- Final Book ---\n";

    std::cout << "Symbols: "
              << book.symbol_count()
              << '\n';

    std::cout << "Has AAPL: "
              << std::boolalpha
              << book.has_symbol(SYMBOL)
              << '\n';

    std::cout << "Has bids: "
              << book.has_bids(SYMBOL)
              << '\n';

    std::cout << "Has asks: "
              << book.has_asks(SYMBOL)
              << '\n';

    if (book.has_bids(SYMBOL)) {
        std::cout << "Best bid: "
                  << book.best_bid(SYMBOL)
                  << '\n';
    }

    if (book.has_asks(SYMBOL)) {
        std::cout << "Best ask: "
                  << book.best_ask(SYMBOL)
                  << '\n';
    }

    return 0;
}
