#ifndef ORDER_BOOK
#define ORDER_BOOK

#include "parser_logic.h"
#include <unordered_map>
#include <map>
#include <iostream>
#include <cstdint>
#include <vector>

struct OrderBookNode{
    uint64_t shares = 0;
};

enum class OrderStatus: uint8_t {
    ADDED, 
    PARTIAL_EXECUTED, 
    EXECUTED,
    CANCELLED
};

void error_with_log(std::string_view s);

struct Order{
    uint64_t ref;
    std::string sym;
    uint16_t stock_locate;
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
        std::cout<<"#########################################"<<std::endl;
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
        if (!bid.empty() && !ask.empty() && ask.back().first <= bid.front().first){
            error_with_log("Error: last ask is less then first bid");
        }
    }
};

class OrderBook {
    public:
        OrderBook(uint16_t max_stock_locate);
        ~OrderBook() = default;
        void add_order(AddOrder &o);
        void execute_order(ExecutedOrder &e);
        void execute_order_with_price(ExecutedWithPriceOrder &e);
        void cancel_order(CancelOrder &c);
        void delete_order(DeleteOrder &d);
        void replace_order(ReplaceOrder &r);
        int get_index(Order &ord);
        bool is_hot_storage(int idx);

        book_map get_book(uint16_t stock_locate, int levels);


    private:
        std::unordered_map<uint64_t, Order> order_map;
        std::vector<std::array<uint32_t, 20000>> bid;
        std::vector<std::array<uint32_t, 20000>> ask;
        std::vector<uint32_t> base;
        std::map<uint16_t, std::map<uint32_t, uint32_t>> cold_bid;
        std::map<uint16_t, std::map<uint32_t, uint32_t>> cold_ask;
};

#endif