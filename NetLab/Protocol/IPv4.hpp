#pragma once

#include <cstdint>
#include <string>

namespace netlab::ipv4 {

bool parse(const std::string& text, std::uint32_t& value) noexcept;
std::string format(std::uint32_t value);
bool isValidSubnetMask(const std::string& text) noexcept;
bool sameSubnet(const std::string& firstAddress,
                const std::string& secondAddress,
                const std::string& subnetMask) noexcept;

}  // namespace netlab::ipv4
