#pragma once

#include "types.hpp"

namespace order_book {

struct PriceLevel {
    types::QtyT total_qty = 0;
    types::Order* head = nullptr;
    types::Order* tail = nullptr;
};

} // namespace order_book
