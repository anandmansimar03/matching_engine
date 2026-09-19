#pragma once

#include "types.hpp"

#include <unordered_map>
#include <vector>

namespace order_book {

class OrderPool {
private:
    types::Order* _free_head = nullptr;
    std::vector<types::Order> _slots;
    std::unordered_map<types::OrderIdT, types::Order*> _lookup_by_id;

public:
    explicit OrderPool(const size_t capacity);

    types::Order* find(const types::OrderIdT id);
    types::Order* allocate(const types::Order& order);
    void free(types::Order* order);
};

} // namespace order_book
