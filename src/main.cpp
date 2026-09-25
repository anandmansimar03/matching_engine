#include "include/global_order_book.hpp"

#include <cassert>
#include <iostream>
#include <limits>
#include <string>

namespace {

using types::CancelReasonEnum;
using types::Order;
using types::OrderIdT;
using types::OrderTypeEnum;
using types::PriceT;
using types::QtyT;
using types::SideEnum;
using types::SymbolT;
using types::Trade;

constexpr std::size_t ORDER_POOL_CAPACITY = 1024;

const SymbolT AAPL = "AAPL";
const SymbolT MSFT = "MSFT";


// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

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
    };
}


const char* side_to_string(const SideEnum side)
{
    switch (side) {
        case SideEnum::Bid:
            return "BID";

        case SideEnum::Ask:
            return "ASK";
    }

    return "UNKNOWN";
}


const char* cancel_reason_to_string(const CancelReasonEnum reason)
{
    switch (reason) {
        case CancelReasonEnum::DuplicateOrder:
            return "DuplicateOrder";

        case CancelReasonEnum::NoLiquidity:
            return "NoLiquidity";

        case CancelReasonEnum::OrderPoolFull:
            return "OrderPoolFull";

        case CancelReasonEnum::UserRequested:
            return "UserRequested";

        case CancelReasonEnum::UnknownOrder:
            return "UnknownOrder";

        case CancelReasonEnum::InvalidQuantity:
            return "InvalidQuantity";
    }

    return "UnknownReason";
}


void print_trade(const SymbolT& symbol, const Trade& trade)
{
    std::cout
        << "  TRADE"
        << " symbol=" << symbol
        << " seq=" << trade.seq_num
        << " price=" << trade.price
        << " qty=" << trade.qty
        << " buyer=" << trade.buyer_id
        << " seller=" << trade.seller_id
        << '\n';
}


void print_result(
    const SymbolT& symbol,
    const types::MatchResult& result)
{
    for (const auto& trade : result.trades) {
        print_trade(symbol, trade);
    }

    for (const auto& cancel : result.cancels) {
        std::cout
            << "  CANCEL"
            << " symbol=" << symbol
            << " seq=" << cancel.seq_num
            << " order_id=" << cancel.order_id
            << " qty=" << cancel.qty
            << " original_qty=" << cancel.original_qty
            << " reason=" << cancel_reason_to_string(
                   cancel.cancel_reason)
            << '\n';
    }

    if (result.trades.empty() && result.cancels.empty()) {
        std::cout << "  No L1 event\n";
    }
}


void print_book(
    const order_book::GlobalOrderBook& book,
    const SymbolT& symbol)
{
    std::cout << "  " << symbol << " BOOK\n";

    if (book.has_bids(symbol)) {
        std::cout
            << "    Best Bid: "
            << book.best_bid(symbol)
            << '\n';
    }
    else {
        std::cout << "    Best Bid: NONE\n";
    }

    if (book.has_asks(symbol)) {
        std::cout
            << "    Best Ask: "
            << book.best_ask(symbol)
            << '\n';
    }
    else {
        std::cout << "    Best Ask: NONE\n";
    }
}


void print_level(
    const order_book::GlobalOrderBook& book,
    const SymbolT& symbol,
    const SideEnum side,
    const PriceT price)
{
    std::cout
        << "    "
        << side_to_string(side)
        << " @ "
        << price
        << " qty="
        << book.qty_at(symbol, side, price)
        << '\n';
}


} // namespace


int main()
{
    using namespace types;
    using namespace order_book;

    GlobalOrderBook book{ORDER_POOL_CAPACITY};

    std::cout << "========================================\n";
    std::cout << "       MATCHING ENGINE SIMULATION\n";
    std::cout << "========================================\n\n";


    // =========================================================================
    // 1. ADD RESTING ORDERS
    // =========================================================================

    std::cout << "----------------------------------------\n";
    std::cout << "1. ADD RESTING ORDERS\n";
    std::cout << "----------------------------------------\n";

    /*
        AAPL:

        ASK
        10100 x 100
        10200 x 200

        BID
         9900 x 150
         9800 x 100
    */

    auto result = book.add_order(
        AAPL,
        make_limit_order(
            1001,
            SideEnum::Ask,
            10100,
            100));

    print_result(AAPL, result);

    result = book.add_order(
        AAPL,
        make_limit_order(
            1002,
            SideEnum::Ask,
            10200,
            200));

    print_result(AAPL, result);

    result = book.add_order(
        AAPL,
        make_limit_order(
            2001,
            SideEnum::Bid,
            9900,
            150));

    print_result(AAPL, result);

    result = book.add_order(
        AAPL,
        make_limit_order(
            2002,
            SideEnum::Bid,
            9800,
            100));

    print_result(AAPL, result);

    print_book(book, AAPL);

    print_level(
        book,
        AAPL,
        SideEnum::Ask,
        10100);

    print_level(
        book,
        AAPL,
        SideEnum::Ask,
        10200);

    print_level(
        book,
        AAPL,
        SideEnum::Bid,
        9900);

    print_level(
        book,
        AAPL,
        SideEnum::Bid,
        9800);

    std::cout << '\n';


    // =========================================================================
    // 2. AGGRESSIVE LIMIT BUY
    // =========================================================================

    std::cout << "----------------------------------------\n";
    std::cout << "2. AGGRESSIVE LIMIT BUY\n";
    std::cout << "----------------------------------------\n";

    /*
        Existing:

        ASK
        10100 x 100
        10200 x 200

        Incoming:

        BUY 150 @ 10200

        Expected:

        100 @ 10100 against 1001
         50 @ 10200 against 1002

        Remaining:

        ASK
        10200 x 150
    */

    result = book.add_order(
        AAPL,
        make_limit_order(
            3001,
            SideEnum::Bid,
            10200,
            150));

    print_result(AAPL, result);

    assert(result.trades.size() == 2);
    assert(result.trades[0].price == 10100);
    assert(result.trades[0].qty == 100);
    assert(result.trades[1].price == 10200);
    assert(result.trades[1].qty == 50);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Ask,
            10200) == 150);

    print_book(book, AAPL);

    std::cout << '\n';


    // =========================================================================
    // 3. PRICE-TIME PRIORITY
    // =========================================================================

    std::cout << "----------------------------------------\n";
    std::cout << "3. PRICE-TIME PRIORITY\n";
    std::cout << "----------------------------------------\n";

    /*
        First consume the remaining 10200 ask so that the next
        matching test is isolated at 10300.

        Remaining before this section:

        ASK
        10200 x 150

        Consume it completely.
    */

    result = book.add_order(
        AAPL,
        make_limit_order(
            3002,
            SideEnum::Bid,
            10200,
            150));

    print_result(AAPL, result);

    assert(result.trades.size() == 1);
    assert(result.trades[0].price == 10200);
    assert(result.trades[0].qty == 150);
    assert(result.trades[0].seller_id == 1002);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Ask,
            10200) == 0);


    /*
        Now add two orders at exactly the same price.

        4001 arrives first.
        4002 arrives second.

        ASK:

        10300 x 50   <- 4001
        10300 x 75   <- 4002

        Incoming:

        BUY 60 @ 10300

        Expected:

        50 @ 10300 against 4001
        10 @ 10300 against 4002

        This directly tests FIFO/time priority.
    */

    result = book.add_order(
        AAPL,
        make_limit_order(
            4001,
            SideEnum::Ask,
            10300,
            50));

    print_result(AAPL, result);

    result = book.add_order(
        AAPL,
        make_limit_order(
            4002,
            SideEnum::Ask,
            10300,
            75));

    print_result(AAPL, result);

    result = book.add_order(
        AAPL,
        make_limit_order(
            3003,
            SideEnum::Bid,
            10300,
            60));

    print_result(AAPL, result);

    assert(result.trades.size() == 2);

    assert(result.trades[0].seller_id == 4001);
    assert(result.trades[0].price == 10300);
    assert(result.trades[0].qty == 50);

    assert(result.trades[1].seller_id == 4002);
    assert(result.trades[1].price == 10300);
    assert(result.trades[1].qty == 10);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Ask,
            10300) == 65);

    print_level(
        book,
        AAPL,
        SideEnum::Ask,
        10300);

    std::cout << '\n';


    // =========================================================================
    // 4. MODIFY ORDER
    // =========================================================================

    std::cout << "----------------------------------------\n";
    std::cout << "4. MODIFY ORDER\n";
    std::cout << "----------------------------------------\n";

    /*
        We modify a resting order by reducing its quantity.

        Existing:

        4002 -> 65 @ 10300

        Modify:

        4002 -> 40 @ 10300

        Expected:

        40 @ 10300

        No L1 event is generated because nothing traded
        and nothing was cancelled.
    */

    result = book.modify_order(
        AAPL,
        4002,
        40);

    print_result(AAPL, result);

    assert(result.trades.empty());
    assert(result.cancels.empty());

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Ask,
            10300) == 40);

    print_level(
        book,
        AAPL,
        SideEnum::Ask,
        10300);

    std::cout << '\n';


    // =========================================================================
    // 5. MODIFY ORDER - INVALID INCREASE
    // =========================================================================

    std::cout << "----------------------------------------\n";
    std::cout << "5. INVALID MODIFY\n";
    std::cout << "----------------------------------------\n";

    /*
        Current:

        4002 -> 40

        Attempt:

        4002 -> 100

        Increasing quantity through MODIFY is rejected.
        REPLACE will eventually be used for this behaviour.
    */

    result = book.modify_order(
        AAPL,
        4002,
        100);

    print_result(AAPL, result);

    assert(result.trades.empty());
    assert(result.cancels.size() == 1);

    assert(
        result.cancels[0].cancel_reason
        == CancelReasonEnum::InvalidQuantity);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Ask,
            10300) == 40);

    std::cout << '\n';


    // =========================================================================
    // 6. MODIFY UNKNOWN ORDER
    // =========================================================================

    std::cout << "----------------------------------------\n";
    std::cout << "6. MODIFY UNKNOWN ORDER\n";
    std::cout << "----------------------------------------\n";

    result = book.modify_order(
        AAPL,
        999999,
        50);

    print_result(AAPL, result);

    assert(result.trades.empty());
    assert(result.cancels.size() == 1);

    assert(
        result.cancels[0].cancel_reason
        == CancelReasonEnum::UnknownOrder);

    std::cout << '\n';


    // =========================================================================
    // 7. CANCEL ORDER
    // =========================================================================

    std::cout << "----------------------------------------\n";
    std::cout << "7. CANCEL ORDER\n";
    std::cout << "----------------------------------------\n";

    /*
        Cancel order 2001:

        BUY 150 @ 9900
    */

    result = book.cancel_order(
        AAPL,
        2001);

    print_result(AAPL, result);

    assert(result.trades.empty());
    assert(result.cancels.size() == 1);

    assert(
        result.cancels[0].cancel_reason
        == CancelReasonEnum::UserRequested);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Bid,
            9900) == 0);

    print_book(book, AAPL);

    std::cout << '\n';


    // =========================================================================
    // 8. CANCEL UNKNOWN ORDER
    // =========================================================================

    std::cout << "----------------------------------------\n";
    std::cout << "8. CANCEL UNKNOWN ORDER\n";
    std::cout << "----------------------------------------\n";

    result = book.cancel_order(
        AAPL,
        999999);

    print_result(AAPL, result);

    assert(result.trades.empty());
    assert(result.cancels.size() == 1);

    assert(
        result.cancels[0].cancel_reason
        == CancelReasonEnum::UnknownOrder);

    std::cout << '\n';


    // =========================================================================
    // 9. MARKET ORDER WITH LIQUIDITY
    // =========================================================================

    std::cout << "----------------------------------------\n";
    std::cout << "9. MARKET ORDER\n";
    std::cout << "----------------------------------------\n";

    /*
        Remaining asks:

        10200 x 150
        10300 x 40

        Incoming:

        BUY 100 MARKET

        Expected:

        100 @ 10200
    */

    result = book.add_order(
        AAPL,
        make_market_order(
            5001,
            SideEnum::Bid,
            100));

    print_result(AAPL, result);

    assert(result.trades.size() == 1);
    assert(result.trades[0].price == 10200);
    assert(result.trades[0].qty == 100);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Ask,
            10200) == 50);

    print_book(book, AAPL);

    std::cout << '\n';


    // =========================================================================
    // 10. MARKET ORDER WITHOUT ENOUGH LIQUIDITY
    // =========================================================================

    std::cout << "----------------------------------------\n";
    std::cout << "10. MARKET ORDER WITHOUT ENOUGH LIQUIDITY\n";
    std::cout << "----------------------------------------\n";

    /*
        Remaining:

        ASK 10200 x 50
        ASK 10300 x 40

        Incoming:

        BUY 200 MARKET

        Expected:

        50 @ 10200
        40 @ 10300
        110 cancelled because there is no liquidity.
    */

    result = book.add_order(
        AAPL,
        make_market_order(
            5002,
            SideEnum::Bid,
            200));

    print_result(AAPL, result);

    assert(result.trades.size() == 2);
    assert(result.trades[0].price == 10200);
    assert(result.trades[0].qty == 50);
    assert(result.trades[1].price == 10300);
    assert(result.trades[1].qty == 40);

    assert(result.cancels.size() == 1);
    assert(
        result.cancels[0].cancel_reason
        == CancelReasonEnum::NoLiquidity);

    assert(result.cancels[0].qty == 110);

    print_book(book, AAPL);

    std::cout << '\n';


    // =========================================================================
    // 11. INVALID QUANTITY
    // =========================================================================

    std::cout << "----------------------------------------\n";
    std::cout << "11. INVALID QUANTITY\n";
    std::cout << "----------------------------------------\n";

    result = book.add_order(
        AAPL,
        make_limit_order(
            6001,
            SideEnum::Bid,
            10000,
            0));

    print_result(AAPL, result);

    assert(result.trades.empty());
    assert(result.cancels.size() == 1);

    assert(
        result.cancels[0].cancel_reason
        == CancelReasonEnum::InvalidQuantity);

    std::cout << '\n';


    // =========================================================================
    // 12. DUPLICATE ORDER ID
    // =========================================================================

    std::cout << "----------------------------------------\n";
    std::cout << "12. DUPLICATE ORDER ID\n";
    std::cout << "----------------------------------------\n";

    /*
        First order.
    */

    result = book.add_order(
        AAPL,
        make_limit_order(
            7001,
            SideEnum::Bid,
            9700,
            100));

    print_result(AAPL, result);

    /*
        Same order ID again.
    */

    result = book.add_order(
        AAPL,
        make_limit_order(
            7001,
            SideEnum::Bid,
            9600,
            200));

    print_result(AAPL, result);

    assert(result.trades.empty());
    assert(result.cancels.size() == 1);

    assert(
        result.cancels[0].cancel_reason
        == CancelReasonEnum::DuplicateOrder);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Bid,
            9700) == 100);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Bid,
            9600) == 0);

    std::cout << '\n';


    // =========================================================================
    // 13. MULTIPLE SYMBOLS
    // =========================================================================

    std::cout << "----------------------------------------\n";
    std::cout << "13. MULTIPLE SYMBOLS\n";
    std::cout << "----------------------------------------\n";

    /*
        Verify that AAPL and MSFT maintain independent order books.
    */

    result = book.add_order(
        MSFT,
        make_limit_order(
            8001,
            SideEnum::Bid,
            50000,
            100));

    print_result(MSFT, result);

    result = book.add_order(
        MSFT,
        make_limit_order(
            8002,
            SideEnum::Ask,
            50100,
            50));

    print_result(MSFT, result);

    assert(book.has_symbol(AAPL));
    assert(book.has_symbol(MSFT));

    assert(
        book.best_bid(MSFT) == 50000);

    assert(
        book.best_ask(MSFT) == 50100);

    assert(
        book.qty_at(
            MSFT,
            SideEnum::Bid,
            50000) == 100);

    assert(
        book.qty_at(
            MSFT,
            SideEnum::Ask,
            50100) == 50);

    print_book(book, MSFT);

    std::cout << '\n';


    // =========================================================================
    // 14. CROSS SYMBOL MATCHING
    // =========================================================================

    std::cout << "----------------------------------------\n";
    std::cout << "14. CROSS SYMBOL MATCHING\n";
    std::cout << "----------------------------------------\n";

    /*
        MSFT:

        BID 50000 x 100
        ASK 50100 x 50

        Incoming:

        BUY 25 @ 50100

        This should NOT affect AAPL.
    */

    result = book.add_order(
        MSFT,
        make_limit_order(
            8003,
            SideEnum::Bid,
            50100,
            25));

    print_result(MSFT, result);

    assert(result.trades.size() == 1);
    assert(result.trades[0].price == 50100);
    assert(result.trades[0].qty == 25);

    assert(
        book.qty_at(
            MSFT,
            SideEnum::Ask,
            50100) == 25);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Bid,
            9700) == 100);

    std::cout << '\n';


    // =========================================================================
    // FINAL STATE
    // =========================================================================

    std::cout << "========================================\n";
    std::cout << "             FINAL STATE\n";
    std::cout << "========================================\n";

    std::cout
        << "Symbols: "
        << book.symbol_count()
        << '\n';

    print_book(book, AAPL);
    print_book(book, MSFT);

    std::cout << '\n';

    std::cout << "========================================\n";
    std::cout << "       ALL TESTS PASSED SUCCESSFULLY\n";
    std::cout << "========================================\n";

    return 0;
}
