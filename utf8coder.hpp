#include<vector>

std::vector<uint8_t> numberToUtf8(uint16_t number) {
    std::vector<uint8_t> result;
    if (number >= 0 && number <= 127) {
        // ASCII range
        result.push_back((uint8_t)number);
    } else if (number >= 128 && number <= 2047) {
        // 2-byte sequence
        result.push_back(((number >> 6) & 0x1F) | 0xC0);
        result.push_back((number & 0x3F) | 0x80);
    } else if (number >= 2048 && number <= 65535) {
        // 3-byte sequence
        result.push_back(((number >> 12) & 0x0F) | 0xE0);
        result.push_back(((number >> 6) & 0x3F) | 0x80);
        result.push_back((number & 0x3F) | 0x80);
    } else if (number >= 65536 && number <= 1114111) {
        // 4-byte sequence
        result.push_back(((number >> 18) & 0x07) | 0xF0);
        result.push_back(((number >> 12) & 0x3F) | 0x80);
        result.push_back(((number >> 6) & 0x3F) | 0x80);
        result.push_back((number & 0x3F) | 0x80);
    } else {
        throw std::runtime_error("Invalid number");
    }
    return result;
}
