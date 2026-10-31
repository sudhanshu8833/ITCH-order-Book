#ifndef ORDER_BOOK
#define ORDER_BOOK

#include "parser_logic.h"
#include <unordered_map>
#include <map>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <string_view>
#include <vector>

struct OrderBookNode{
    uint64_t shares = 0;
    uint32_t price = 0;
};

enum class OrderStatus: uint8_t {
    ADDED, 
    PARTIAL_EXECUTED, 
    EXECUTED,
    CANCELLED
};

void error_with_log(std::string_view s);

struct Order{
    uint16_t stock_locate;
    uint64_t ref;
    uint64_t add_timestamp;
    uint64_t execute_timestamp = 0;
    uint64_t delete_timestamp = 0;
    uint32_t limit_price;
    uint32_t price_executed = 0;
    OrderStatus status = OrderStatus::ADDED;
    char side;
    uint32_t add_shares;
    uint32_t execute_shares = 0;
    uint32_t cancel_shares = 0;

    inline uint64_t available_shares() const {
        return static_cast<int64_t>(this->add_shares) - this->execute_shares - this->cancel_shares;
    }
};

struct book_map {
    std::vector<std::pair<uint32_t, uint32_t>> bid = {};
    std::vector<std::pair<uint32_t, uint32_t>> ask = {};

    inline void print_struct(){
        // std::cout<<"#########################################"<<std::endl;
        int count = 1;
        for(auto a: ask){
            std::cout<< "ASK "<< count<<": price: "<< (a.first / 10000.0)<<" quantity: "<< a.second<<std::endl;
            count++;
        }
        std::cout<<std::endl;

        count = 1;
        for(auto b: bid){
            std::cout<< "BID "<< count<<": price: "<< (b.first / 10000.0)<<" quantity: "<< b.second<<std::endl;
            count++;
        }
        std::cout<<std::endl;
    }

    inline void validate_book(){
        // bid.front() is the best bid and ask.front() the best ask; a book is crossed when they meet.
        if (!bid.empty() && !ask.empty() && ask.front().first <= bid.front().first){
            error_with_log("Error: best ask is not above best bid");
        }
    }
};

class OrderBook {
    public:
        OrderBook(int max_stock_locate);
        ~OrderBook() = default;
        void add_order(AddOrder &o);
        void execute_order(ExecutedOrder &e);
        void execute_order_with_price(ExecutedWithPriceOrder &e);
        void cancel_order(CancelOrder &c);
        void delete_order(DeleteOrder &d);
        void replace_order(ReplaceOrder &r);

        book_map get_book(uint16_t stock_locate, int levels);
        size_t live_orders() const { return order_map.size(); }
        void add_to_book(std::vector<OrderBookNode> &book_array, uint32_t limit_price, uint32_t shares); 
        void delete_from_book(std::vector<OrderBookNode> &book_array, uint32_t limit_price, uint32_t shares);

    private:
        std::unordered_map<uint64_t, Order> order_map;
        std::vector<std::vector<OrderBookNode>> bid;
        std::vector<std::vector<OrderBookNode>> ask;
};

#endif