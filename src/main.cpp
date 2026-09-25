#include "include/global_order_book.hpp"

#include <cassert>
#include <iostream>
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

const SymbolT TEST = "TEST";
const SymbolT REPLACE = "REPLACE";
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


const char* cancel_reason_to_string(
    const CancelReasonEnum reason)
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


void print_trade(
    const SymbolT& symbol,
    const Trade& trade)
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
            << " reason="
            << cancel_reason_to_string(
                   cancel.cancel_reason)
            << '\n';
    }

    if (result.trades.empty() &&
        result.cancels.empty()) {
        std::cout << "  No L1 event\n";
    }
}


void print_book(
    const order_book::GlobalOrderBook& book,
    const SymbolT& symbol)
{
    std::cout
        << "  "
        << symbol
        << " BOOK\n";

    if (book.has_bids(symbol)) {
        std::cout
            << "    Best Bid: "
            << book.best_bid(symbol)
            << '\n';
    }
    else {
        std::cout
            << "    Best Bid: NONE\n";
    }

    if (book.has_asks(symbol)) {
        std::cout
            << "    Best Ask: "
            << book.best_ask(symbol)
            << '\n';
    }
    else {
        std::cout
            << "    Best Ask: NONE\n";
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
        << book.qty_at(
               symbol,
               side,
               price)
        << '\n';
}


} // namespace


int main()
{
    using namespace types;
    using namespace order_book;

    GlobalOrderBook book{ORDER_POOL_CAPACITY};

    std::cout
        << "========================================\n";

    std::cout
        << "       MATCHING ENGINE SIMULATION\n";

    std::cout
        << "========================================\n\n";


    // =========================================================================
    // 1. ADD RESTING ORDERS
    // =========================================================================

    std::cout
        << "----------------------------------------\n";

    std::cout
        << "1. ADD RESTING ORDERS\n";

    std::cout
        << "----------------------------------------\n";

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

    assert(result.trades.empty());
    assert(result.cancels.empty());

    result = book.add_order(
        AAPL,
        make_limit_order(
            1002,
            SideEnum::Ask,
            10200,
            200));

    print_result(AAPL, result);

    assert(result.trades.empty());
    assert(result.cancels.empty());

    result = book.add_order(
        AAPL,
        make_limit_order(
            2001,
            SideEnum::Bid,
            9900,
            150));

    print_result(AAPL, result);

    assert(result.trades.empty());
    assert(result.cancels.empty());

    result = book.add_order(
        AAPL,
        make_limit_order(
            2002,
            SideEnum::Bid,
            9800,
            100));

    print_result(AAPL, result);

    assert(result.trades.empty());
    assert(result.cancels.empty());

    assert(book.has_bids(AAPL));
    assert(book.has_asks(AAPL));

    assert(book.best_bid(AAPL) == 9900);
    assert(book.best_ask(AAPL) == 10100);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Ask,
            10100) == 100);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Ask,
            10200) == 200);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Bid,
            9900) == 150);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Bid,
            9800) == 100);

    print_book(book, AAPL);

    std::cout << '\n';


    // =========================================================================
    // 2. AGGRESSIVE LIMIT BUY
    // =========================================================================

    std::cout
        << "----------------------------------------\n";

    std::cout
        << "2. AGGRESSIVE LIMIT BUY\n";

    std::cout
        << "----------------------------------------\n";

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
    assert(result.cancels.empty());

    assert(result.trades[0].price == 10100);
    assert(result.trades[0].qty == 100);
    assert(result.trades[0].buyer_id == 3001);
    assert(result.trades[0].seller_id == 1001);

    assert(result.trades[1].price == 10200);
    assert(result.trades[1].qty == 50);
    assert(result.trades[1].buyer_id == 3001);
    assert(result.trades[1].seller_id == 1002);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Ask,
            10100) == 0);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Ask,
            10200) == 150);

    assert(book.best_ask(AAPL) == 10200);

    print_book(book, AAPL);

    std::cout << '\n';


    // =========================================================================
    // 3. PRICE-TIME PRIORITY
    // =========================================================================

    std::cout
        << "----------------------------------------\n";

    std::cout
        << "3. PRICE-TIME PRIORITY\n";

    std::cout
        << "----------------------------------------\n";

    /*
        Remove the remaining 10200 liquidity first.
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
    assert(result.trades[0].buyer_id == 3002);
    assert(result.trades[0].seller_id == 1002);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Ask,
            10200) == 0);

    /*
        Add two orders at exactly the same price.

        FIFO queue:

        10300:

        4001 x 50
        4002 x 75

        Then:

        BUY 60 @ 10300

        Expected:

        50 against 4001
        10 against 4002
    */

    result = book.add_order(
        AAPL,
        make_limit_order(
            4001,
            SideEnum::Ask,
            10300,
            50));

    print_result(AAPL, result);

    assert(result.trades.empty());
    assert(result.cancels.empty());

    result = book.add_order(
        AAPL,
        make_limit_order(
            4002,
            SideEnum::Ask,
            10300,
            75));

    print_result(AAPL, result);

    assert(result.trades.empty());
    assert(result.cancels.empty());

    result = book.add_order(
        AAPL,
        make_limit_order(
            3003,
            SideEnum::Bid,
            10300,
            60));

    print_result(AAPL, result);

    assert(result.trades.size() == 2);
    assert(result.cancels.empty());

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

    std::cout
        << "----------------------------------------\n";

    std::cout
        << "4. MODIFY ORDER\n";

    std::cout
        << "----------------------------------------\n";

    /*
        Current:

        4002 -> 65 @ 10300

        MODIFY:

        4002 -> 40 @ 10300

        Expected:

        40 @ 10300

        No L1 event.
        FIFO position is preserved.
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

    /*
        MODIFY to the same quantity is a no-op.
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
    // 5. MODIFY - INVALID INCREASE
    // =========================================================================

    std::cout
        << "----------------------------------------\n";

    std::cout
        << "5. MODIFY - INVALID INCREASE\n";

    std::cout
        << "----------------------------------------\n";

    /*
        Current:

        4002 -> 40

        Attempt:

        4002 -> 100

        MODIFY may only reduce quantity.
    */

    result = book.modify_order(
        AAPL,
        4002,
        100);

    print_result(AAPL, result);

    assert(result.trades.empty());
    assert(result.cancels.size() == 1);

    assert(
        result.cancels[0].order_id == 4002);

    assert(
        result.cancels[0].qty == 40);

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
    // 6. MODIFY - UNKNOWN ORDER
    // =========================================================================

    std::cout
        << "----------------------------------------\n";

    std::cout
        << "6. MODIFY - UNKNOWN ORDER\n";

    std::cout
        << "----------------------------------------\n";

    result = book.modify_order(
        AAPL,
        999999,
        50);

    print_result(AAPL, result);

    assert(result.trades.empty());
    assert(result.cancels.size() == 1);

    assert(
        result.cancels[0].order_id == 999999);

    assert(
        result.cancels[0].cancel_reason
        == CancelReasonEnum::UnknownOrder);

    std::cout << '\n';


    // =========================================================================
    // 7. REPLACE ORDER
    // =========================================================================

    std::cout
        << "----------------------------------------\n";

    std::cout
        << "7. REPLACE ORDER\n";

    std::cout
        << "----------------------------------------\n";

    /*
        Current:

        4002 -> 40 @ 10300

        REPLACE:

        4002 -> 70 @ 10400

        Expected:

        10300 level disappears.
        10400 x 70 is created.

        No L1 event because the replacement is resting.
    */

    result = book.replace_order(
        AAPL,
        4002,
        10400,
        70);

    print_result(AAPL, result);

    assert(result.trades.empty());
    assert(result.cancels.empty());

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Ask,
            10300) == 0);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Ask,
            10400) == 70);

    assert(book.best_ask(AAPL) == 10400);

    print_book(book, AAPL);

    std::cout << '\n';


    // =========================================================================
    // 8. REPLACE - INVALID QUANTITY
    // =========================================================================

    std::cout
        << "----------------------------------------\n";

    std::cout
        << "8. REPLACE - INVALID QUANTITY\n";

    std::cout
        << "----------------------------------------\n";

    /*
        Current:

        4002 -> 70 @ 10400

        Invalid replacement:

        4002 -> 0 @ 10400

        Expected:

        Reject.
        Existing order remains untouched.
    */

    result = book.replace_order(
        AAPL,
        4002,
        10400,
        0);

    print_result(AAPL, result);

    assert(result.trades.empty());
    assert(result.cancels.size() == 1);

    assert(
        result.cancels[0].order_id == 4002);

    assert(
        result.cancels[0].qty == 70);

    assert(
        result.cancels[0].original_qty == 70);

    assert(
        result.cancels[0].cancel_reason
        == CancelReasonEnum::InvalidQuantity);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Ask,
            10400) == 70);

    std::cout << '\n';


    // =========================================================================
    // 9. REPLACE - UNKNOWN ORDER
    // =========================================================================

    std::cout
        << "----------------------------------------\n";

    std::cout
        << "9. REPLACE - UNKNOWN ORDER\n";

    std::cout
        << "----------------------------------------\n";

    result = book.replace_order(
        AAPL,
        888888,
        10500,
        50);

    print_result(AAPL, result);

    assert(result.trades.empty());
    assert(result.cancels.size() == 1);

    assert(
        result.cancels[0].order_id == 888888);

    assert(
        result.cancels[0].cancel_reason
        == CancelReasonEnum::UnknownOrder);

    std::cout << '\n';


    // =========================================================================
    // 10. REPLACE RESETS FIFO PRIORITY
    // =========================================================================

    std::cout
        << "----------------------------------------\n";

    std::cout
        << "10. REPLACE RESETS FIFO PRIORITY\n";

    std::cout
        << "----------------------------------------\n";

    /*
        Use a dedicated symbol so this test is isolated.

        Create:

        ASK @ 10500

        4101 x 50
        4102 x 50

        Original queue:

        4101 -> 4102


        Replace 4101:

        4101 x 60 @ 10500

        Expected queue:

        4102 -> 4101


        Incoming:

        BUY 60 @ 10500

        Expected:

        50 @ 10500 against 4102
        10 @ 10500 against 4101
    */

    result = book.add_order(
        TEST,
        make_limit_order(
            4101,
            SideEnum::Ask,
            10500,
            50));

    print_result(TEST, result);

    assert(result.trades.empty());
    assert(result.cancels.empty());

    result = book.add_order(
        TEST,
        make_limit_order(
            4102,
            SideEnum::Ask,
            10500,
            50));

    print_result(TEST, result);

    assert(result.trades.empty());
    assert(result.cancels.empty());

    assert(
        book.qty_at(
            TEST,
            SideEnum::Ask,
            10500) == 100);

    result = book.replace_order(
        TEST,
        4101,
        10500,
        60);

    print_result(TEST, result);

    assert(result.trades.empty());
    assert(result.cancels.empty());

    assert(
        book.qty_at(
            TEST,
            SideEnum::Ask,
            10500) == 110);

    /*
        BUY 60 @ 10500

        Correct FIFO result:

        50 -> 4102
        10 -> 4101
    */

    result = book.add_order(
        TEST,
        make_limit_order(
            4201,
            SideEnum::Bid,
            10500,
            60));

    print_result(TEST, result);

    assert(result.trades.size() == 2);
    assert(result.cancels.empty());

    assert(
        result.trades[0].seller_id == 4102);

    assert(
        result.trades[0].price == 10500);

    assert(
        result.trades[0].qty == 50);

    assert(
        result.trades[1].seller_id == 4101);

    assert(
        result.trades[1].price == 10500);

    assert(
        result.trades[1].qty == 10);

    /*
        Remaining:

        4101 x 50 @ 10500
    */

    assert(
        book.qty_at(
            TEST,
            SideEnum::Ask,
            10500) == 50);

    assert(book.best_ask(TEST) == 10500);

    print_book(book, TEST);

    print_level(
        book,
        TEST,
        SideEnum::Ask,
        10500);

    std::cout << '\n';


    // =========================================================================
    // 11. REPLACE THAT IMMEDIATELY MATCHES
    // =========================================================================

    std::cout
        << "----------------------------------------\n";

    std::cout
        << "11. REPLACE THAT IMMEDIATELY MATCHES\n";

    std::cout
        << "----------------------------------------\n";

    /*
        Use a dedicated symbol so this test is isolated.

        Create:

        BID 10700 x 20
        ASK 10800 x 30

        Then replace the ask:

        ASK 10600 x 30

        Because 10600 <= best bid 10700,
        the replacement immediately matches.

        Expected:

        20 @ 10700

        Remaining:

        ASK 10600 x 10
    */

    result = book.add_order(
        REPLACE,
        make_limit_order(
            4301,
            SideEnum::Bid,
            10700,
            20));

    print_result(REPLACE, result);

    assert(result.trades.empty());
    assert(result.cancels.empty());

    assert(
        book.qty_at(
            REPLACE,
            SideEnum::Bid,
            10700) == 20);

    result = book.add_order(
        REPLACE,
        make_limit_order(
            4302,
            SideEnum::Ask,
            10800,
            30));

    print_result(REPLACE, result);

    assert(result.trades.empty());
    assert(result.cancels.empty());

    assert(
        book.qty_at(
            REPLACE,
            SideEnum::Ask,
            10800) == 30);

    /*
        Replace 4302 from 10800 -> 10600.

        This crosses the 10700 bid and therefore
        immediately executes.
    */

    result = book.replace_order(
        REPLACE,
        4302,
        10600,
        30);

    print_result(REPLACE, result);

    assert(result.cancels.empty());
    assert(result.trades.size() == 1);

    assert(
        result.trades[0].price == 10700);

    assert(
        result.trades[0].qty == 20);

    assert(
        result.trades[0].buyer_id == 4301);

    assert(
        result.trades[0].seller_id == 4302);

    assert(
        book.qty_at(
            REPLACE,
            SideEnum::Bid,
            10700) == 0);

    assert(
        book.qty_at(
            REPLACE,
            SideEnum::Ask,
            10600) == 10);

    assert(
        book.qty_at(
            REPLACE,
            SideEnum::Ask,
            10800) == 0);

    print_book(book, REPLACE);

    std::cout << '\n';


    // =========================================================================
    // 12. CANCEL ORDER
    // =========================================================================

    std::cout
        << "----------------------------------------\n";

    std::cout
        << "12. CANCEL ORDER\n";

    std::cout
        << "----------------------------------------\n";

    /*
        Cancel 4002:

        ASK 10400 x 70

        Expected:

        - order removed
        - price level removed
        - Cancel(UserRequested)
    */

    result = book.cancel_order(
        AAPL,
        4002);

    print_result(AAPL, result);

    assert(result.trades.empty());
    assert(result.cancels.size() == 1);

    assert(
        result.cancels[0].order_id == 4002);

    assert(
        result.cancels[0].qty == 70);

    assert(
        result.cancels[0].original_qty == 70);

    assert(
        result.cancels[0].cancel_reason
        == CancelReasonEnum::UserRequested);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Ask,
            10400) == 0);

    assert(
        !book.has_asks(AAPL));

    print_book(book, AAPL);

    std::cout << '\n';


    // =========================================================================
    // 13. CANCEL FIFO QUEUE - HEAD / MIDDLE / TAIL
    // =========================================================================

    std::cout
        << "----------------------------------------\n";

    std::cout
        << "13. CANCEL FIFO QUEUE - HEAD / MIDDLE / TAIL\n";

    std::cout
        << "----------------------------------------\n";

    /*
        Create one FIFO queue:

        11000:

        5101 x 30
        5102 x 40
        5103 x 50

        Test middle removal, then head, then tail.
    */

    result = book.add_order(
        AAPL,
        make_limit_order(
            5101,
            SideEnum::Ask,
            11000,
            30));

    print_result(AAPL, result);

    result = book.add_order(
        AAPL,
        make_limit_order(
            5102,
            SideEnum::Ask,
            11000,
            40));

    print_result(AAPL, result);

    result = book.add_order(
        AAPL,
        make_limit_order(
            5103,
            SideEnum::Ask,
            11000,
            50));

    print_result(AAPL, result);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Ask,
            11000) == 120);

    /*
        Remove middle.
    */

    result = book.cancel_order(
        AAPL,
        5102);

    print_result(AAPL, result);

    assert(result.cancels.size() == 1);

    assert(
        result.cancels[0].order_id == 5102);

    assert(
        result.cancels[0].qty == 40);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Ask,
            11000) == 80);

    /*
        Remove head.
    */

    result = book.cancel_order(
        AAPL,
        5101);

    print_result(AAPL, result);

    assert(result.cancels.size() == 1);

    assert(
        result.cancels[0].order_id == 5101);

    assert(
        result.cancels[0].qty == 30);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Ask,
            11000) == 50);

    /*
        Remove tail.
    */

    result = book.cancel_order(
        AAPL,
        5103);

    print_result(AAPL, result);

    assert(result.cancels.size() == 1);

    assert(
        result.cancels[0].order_id == 5103);

    assert(
        result.cancels[0].qty == 50);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Ask,
            11000) == 0);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Ask,
            11000) == 0);

    std::cout << '\n';


    // =========================================================================
    // 14. CANCEL UNKNOWN ORDER
    // =========================================================================

    std::cout
        << "----------------------------------------\n";

    std::cout
        << "14. CANCEL UNKNOWN ORDER\n";

    std::cout
        << "----------------------------------------\n";

    result = book.cancel_order(
        AAPL,
        999999);

    print_result(AAPL, result);

    assert(result.trades.empty());
    assert(result.cancels.size() == 1);

    assert(
        result.cancels[0].order_id == 999999);

    assert(
        result.cancels[0].qty == 0);

    assert(
        result.cancels[0].original_qty == 0);

    assert(
        result.cancels[0].cancel_reason
        == CancelReasonEnum::UnknownOrder);

    std::cout << '\n';


    // =========================================================================
    // 15. MARKET ORDER WITH LIQUIDITY
    // =========================================================================

    std::cout
        << "----------------------------------------\n";

    std::cout
        << "15. MARKET ORDER WITH LIQUIDITY\n";

    std::cout
        << "----------------------------------------\n";

    /*
        Add fresh AAPL liquidity:

        ASK
        10200 x 150
        10300 x 40

        Incoming:

        BUY 100 MARKET

        Expected:

        100 @ 10200

        Remaining:

        10200 x 50
        10300 x 40
    */

    result = book.add_order(
        AAPL,
        make_limit_order(
            5201,
            SideEnum::Ask,
            10200,
            150));

    print_result(AAPL, result);

    assert(result.trades.empty());
    assert(result.cancels.empty());

    result = book.add_order(
        AAPL,
        make_limit_order(
            5202,
            SideEnum::Ask,
            10300,
            40));

    print_result(AAPL, result);

    assert(result.trades.empty());
    assert(result.cancels.empty());

    result = book.add_order(
        AAPL,
        make_market_order(
            5301,
            SideEnum::Bid,
            100));

    print_result(AAPL, result);

    assert(result.trades.size() == 1);
    assert(result.cancels.empty());

    assert(
        result.trades[0].price == 10200);

    assert(
        result.trades[0].qty == 100);

    assert(
        result.trades[0].buyer_id == 5301);

    assert(
        result.trades[0].seller_id == 5201);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Ask,
            10200) == 50);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Ask,
            10300) == 40);

    print_book(book, AAPL);

    std::cout << '\n';


    // =========================================================================
    // 16. MARKET ORDER WITHOUT ENOUGH LIQUIDITY
    // =========================================================================

    std::cout
        << "----------------------------------------\n";

    std::cout
        << "16. MARKET ORDER WITHOUT ENOUGH LIQUIDITY\n";

    std::cout
        << "----------------------------------------\n";

    /*
        Remaining:

        ASK
        10200 x 50
        10300 x 40

        Incoming:

        BUY 200 MARKET

        Expected:

        50 @ 10200
        40 @ 10300
        110 cancelled due to NoLiquidity.
    */

    result = book.add_order(
        AAPL,
        make_market_order(
            5302,
            SideEnum::Bid,
            200));

    print_result(AAPL, result);

    assert(result.trades.size() == 2);
    assert(result.cancels.size() == 1);

    assert(
        result.trades[0].price == 10200);

    assert(
        result.trades[0].qty == 50);

    assert(
        result.trades[1].price == 10300);

    assert(
        result.trades[1].qty == 40);

    assert(
        result.cancels[0].order_id == 5302);

    assert(
        result.cancels[0].qty == 110);

    assert(
        result.cancels[0].original_qty == 200);

    assert(
        result.cancels[0].cancel_reason
        == CancelReasonEnum::NoLiquidity);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Ask,
            10200) == 0);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Ask,
            10300) == 0);

    assert(!book.has_asks(AAPL));

    print_book(book, AAPL);

    std::cout << '\n';


    // =========================================================================
    // 17. INVALID QUANTITY
    // =========================================================================

    std::cout
        << "----------------------------------------\n";

    std::cout
        << "17. INVALID QUANTITY\n";

    std::cout
        << "----------------------------------------\n";

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
        result.cancels[0].order_id == 6001);

    assert(
        result.cancels[0].qty == 0);

    assert(
        result.cancels[0].original_qty == 0);

    assert(
        result.cancels[0].cancel_reason
        == CancelReasonEnum::InvalidQuantity);

    std::cout << '\n';


    // =========================================================================
    // 18. DUPLICATE ORDER ID
    // =========================================================================

    std::cout
        << "----------------------------------------\n";

    std::cout
        << "18. DUPLICATE ORDER ID\n";

    std::cout
        << "----------------------------------------\n";

    /*
        First order should be accepted.
    */

    result = book.add_order(
        AAPL,
        make_limit_order(
            6101,
            SideEnum::Bid,
            9700,
            100));

    print_result(AAPL, result);

    assert(result.trades.empty());
    assert(result.cancels.empty());

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Bid,
            9700) == 100);

    /*
        Same order ID should be rejected.
    */

    result = book.add_order(
        AAPL,
        make_limit_order(
            6101,
            SideEnum::Bid,
            9600,
            200));

    print_result(AAPL, result);

    assert(result.trades.empty());
    assert(result.cancels.size() == 1);

    assert(
        result.cancels[0].order_id == 6101);

    assert(
        result.cancels[0].qty == 200);

    assert(
        result.cancels[0].original_qty == 200);

    assert(
        result.cancels[0].cancel_reason
        == CancelReasonEnum::DuplicateOrder);

    /*
        Original order must remain untouched.
    */

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
    // 19. MULTIPLE SYMBOLS
    // =========================================================================

    std::cout
        << "----------------------------------------\n";

    std::cout
        << "19. MULTIPLE SYMBOLS\n";

    std::cout
        << "----------------------------------------\n";

    /*
        MSFT has its own independent book.
    */

    result = book.add_order(
        MSFT,
        make_limit_order(
            8001,
            SideEnum::Bid,
            50000,
            100));

    print_result(MSFT, result);

    assert(result.trades.empty());
    assert(result.cancels.empty());

    result = book.add_order(
        MSFT,
        make_limit_order(
            8002,
            SideEnum::Ask,
            50100,
            50));

    print_result(MSFT, result);

    assert(result.trades.empty());
    assert(result.cancels.empty());

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
    // 20. SYMBOL ISOLATION
    // =========================================================================

    std::cout
        << "----------------------------------------\n";

    std::cout
        << "20. SYMBOL ISOLATION\n";

    std::cout
        << "----------------------------------------\n";

    /*
        BUY 25 @ 50100 on MSFT.

        Should match the MSFT ask only.

        AAPL must remain unchanged.
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
    assert(result.cancels.empty());

    assert(
        result.trades[0].price == 50100);

    assert(
        result.trades[0].qty == 25);

    assert(
        result.trades[0].buyer_id == 8003);

    assert(
        result.trades[0].seller_id == 8002);

    assert(
        book.qty_at(
            MSFT,
            SideEnum::Ask,
            50100) == 25);

    /*
        AAPL should still contain:

        BID 9900 x 150
        BID 9800 x 100
        BID 9700 x 100
    */

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Bid,
            9900) == 150);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Bid,
            9800) == 100);

    assert(
        book.qty_at(
            AAPL,
            SideEnum::Bid,
            9700) == 100);

    print_book(book, AAPL);
    print_book(book, MSFT);

    std::cout << '\n';


    // =========================================================================
    // FINAL STATE
    // =========================================================================

    std::cout
        << "========================================\n";

    std::cout
        << "             FINAL STATE\n";

    std::cout
        << "========================================\n";

    std::cout
        << "Symbols: "
        << book.symbol_count()
        << '\n';

    print_book(book, AAPL);

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

    print_level(
        book,
        AAPL,
        SideEnum::Bid,
        9700);

    std::cout << '\n';

    print_book(book, MSFT);

    print_level(
        book,
        MSFT,
        SideEnum::Bid,
        50000);

    print_level(
        book,
        MSFT,
        SideEnum::Ask,
        50100);

    std::cout << '\n';

    print_book(book, TEST);

    print_level(
        book,
        TEST,
        SideEnum::Ask,
        10500);

    std::cout << '\n';

    print_book(book, REPLACE);

    print_level(
        book,
        REPLACE,
        SideEnum::Ask,
        10600);

    std::cout << '\n';

    std::cout
        << "========================================\n";

    std::cout
        << "       ALL TESTS PASSED SUCCESSFULLY\n";

    std::cout
        << "========================================\n";

    return 0;
}
