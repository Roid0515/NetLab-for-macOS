#include "IPv4.hpp"

#include <array>
#include <charconv>
#include <sstream>

namespace netlab::ipv4 {

bool parse(const std::string& text, std::uint32_t& value) noexcept {
    std::array<std::uint32_t, 4> octets{};
    std::size_t start = 0;
    for (std::size_t index = 0; index < octets.size(); ++index) {
        std::size_t end = index == 3 ? text.size() : text.find('.', start);
        if (end == std::string::npos || end == start) return false;
        const char* beginPointer = text.data() + start;
        const char* endPointer = text.data() + end;
        auto result = std::from_chars(beginPointer, endPointer, octets[index]);
        if (result.ec != std::errc{} || result.ptr != endPointer || octets[index] > 255) return false;
        start = end + 1;
    }
    if (start != text.size() + 1) return false;
    value = (octets[0] << 24) | (octets[1] << 16) | (octets[2] << 8) | octets[3];
    return true;
}

std::string format(std::uint32_t value) {
    std::ostringstream stream;
    stream << ((value >> 24) & 0xff) << '.'
           << ((value >> 16) & 0xff) << '.'
           << ((value >> 8) & 0xff) << '.'
           << (value & 0xff);
    return stream.str();
}

bool isValidSubnetMask(const std::string& text) noexcept {
    std::uint32_t mask = 0;
    if (!parse(text, mask) || mask == 0) return false;
    std::uint32_t inverted = ~mask;
    return (inverted & (inverted + 1)) == 0;
}

bool sameSubnet(const std::string& firstAddress,
                const std::string& secondAddress,
                const std::string& subnetMask) noexcept {
    std::uint32_t first = 0;
    std::uint32_t second = 0;
    std::uint32_t mask = 0;
    if (!parse(firstAddress, first) || !parse(secondAddress, second) ||
        !parse(subnetMask, mask) || !isValidSubnetMask(subnetMask)) return false;
    return (first & mask) == (second & mask);
}

}  // namespace netlab::ipv4
