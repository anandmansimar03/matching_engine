#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace types {

using PriceT = int64_t;
using QtyT = int64_t;
using OrderIdT = uint64_t;
using SymbolT = std::string;
using SeqNumT = std::uint64_t;
using TimestampT = uint64_t;

enum class SideEnum : uint8_t {
    Bid,
    Ask
};

enum class OrderTypeEnum : uint8_t {
    Limit,
    Market
};

enum class CancelReasonEnum : uint8_t {
    DuplicateOrder,
    NoLiquidity,
    OrderPoolFull,
    UnknownOrder,
    UserRequested,
};

struct Order {
    OrderIdT order_id;
    SideEnum side;
    OrderTypeEnum order_type;
    PriceT price;
    QtyT qty;
    // set by the book on entry; qty is decremented as the order fills
    QtyT original_qty;
    Order* prev = nullptr;
    Order* next = nullptr;
};

struct Trade {
    SeqNumT seq_num;
    TimestampT timestamp_ns;
    PriceT price;
    QtyT qty;
    OrderIdT buyer_id;
    OrderIdT seller_id;
};

struct Cancel {
    SeqNumT seq_num;
    TimestampT timestamp_ns;
    OrderIdT order_id;
    QtyT qty;
    QtyT original_qty;
    CancelReasonEnum cancel_reason;
};

struct MatchResult {
    std::vector<Trade> trades;
    std::vector<Cancel> cancels;
};

} // namespace types