#ifndef PARSER
#define PARSER

#include <cstdint>
#include <cstddef>

struct AddOrder{
    uint16_t stock_locate;
    uint64_t timestamp;
    std::string stock;
    uint64_t ref;
    uint32_t price;
    uint32_t shares;
    char side;  // B or S;

    void print_struct();

    void validate_info();
};

struct ExecutedOrder{
    uint16_t stock_locate;
    uint64_t timestamp;
    uint64_t ref;
    uint32_t executed_shares;

    void print_struct();

    void validate_info();
};

struct ExecutedWithPriceOrder{
    uint16_t stock_locate;
    uint64_t timestamp;
    uint64_t ref;
    uint32_t executed_shares;
    uint32_t execution_price;

    void print_struct();

    void validate_info();
};

struct CancelOrder{
    uint16_t stock_locate;
    uint64_t timestamp;
    uint64_t ref;
    uint32_t cancelled_shares;

    void print_struct();

    void validate_info();
};

struct DeleteOrder{
    uint16_t stock_locate;
    uint64_t timestamp;
    uint64_t ref;

    void print_struct();

    void validate_info();
};


struct ReplaceOrder{
    uint16_t stock_locate;
    uint64_t timestamp;
    uint64_t ref;
    uint64_t new_ref;
    uint32_t shares;
    uint32_t price;

    void print_struct();

    void validate_info();
};

struct StockDirectory{
    uint16_t stock_locate;
};



uint16_t read_be16(const unsigned char* message);
uint32_t read_be32(const unsigned char* message);
uint64_t read_be64(const unsigned char* message);
uint64_t read_be48(const unsigned char* message);

AddOrder ParseAddOrderMessage(const unsigned char* message);

ExecutedOrder ParseExecutedOrderMessage(const unsigned char* message);

ExecutedWithPriceOrder ParseExecutedWithPriceOrderMessage(const unsigned char* message);

CancelOrder ParseCancelOrder(const unsigned char* message);

DeleteOrder ParseDeleteOrder(const unsigned char* message);

ReplaceOrder ParseReplaceOrder(const unsigned char* message);

uint16_t FindMaxStockLocateCode(const unsigned char* data, size_t size);

#endif