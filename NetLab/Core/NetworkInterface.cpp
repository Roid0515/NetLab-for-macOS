#include "NetworkInterface.hpp"

#include "../Protocol/IPv4.hpp"

#include <arpa/inet.h>

#include <algorithm>
#include <utility>

namespace netlab {

NetworkInterface::NetworkInterface(std::string name, std::string macAddress, int speedMbps)
    : name_(std::move(name)), macAddress_(std::move(macAddress)), speedMbps_(speedMbps) {}

bool NetworkInterface::hasIPv4Configuration() const noexcept {
    return !ipv4Address_.empty() && !subnetMask_.empty();
}

bool NetworkInterface::configureIPv4(const std::string& address, const std::string& subnetMask) {
    if (!isValidIPv4Configuration(address, subnetMask)) return false;
    ipv4Address_ = address;
    subnetMask_ = subnetMask;
    return true;
}

bool NetworkInterface::isValidIPv4Configuration(const std::string& address,
                                                 const std::string& subnetMask) noexcept {
    std::uint32_t ignored = 0;
    return ipv4::parse(address, ignored) && ipv4::isValidSubnetMask(subnetMask);
}

bool NetworkInterface::configureIPv6(const std::string& address, int prefixLength) {
    if (!isValidIPv6Configuration(address, prefixLength)) return false;
    ipv6Address_ = address;
    ipv6PrefixLength_ = prefixLength;
    return true;
}

bool NetworkInterface::isValidIPv6Configuration(const std::string& address,
                                                 int prefixLength) noexcept {
    if (address.empty() || prefixLength < 0 || prefixLength > 128) return false;
    in6_addr parsed{};
    return inet_pton(AF_INET6, address.c_str(), &parsed) == 1;
}

void NetworkInterface::clearIPv6Configuration() noexcept {
    ipv6Address_.clear();
    ipv6PrefixLength_ = 0;
}

bool NetworkInterface::configureAccessVLAN(int vlanID) {
    if (vlanID < 1 || vlanID > 4094) return false;
    switchportMode_ = SwitchportMode::Access;
    accessVLAN_ = vlanID;
    allowedVLANs_ = {vlanID};
    return true;
}

bool NetworkInterface::configureTrunk(int nativeVLAN, std::vector<int> allowedVLANs) {
    if (nativeVLAN < 1 || nativeVLAN > 4094 || allowedVLANs.empty()) return false;
    for (int vlanID : allowedVLANs) if (vlanID < 1 || vlanID > 4094) return false;
    if (std::find(allowedVLANs.begin(), allowedVLANs.end(), nativeVLAN) == allowedVLANs.end()) {
        allowedVLANs.push_back(nativeVLAN);
    }
    std::sort(allowedVLANs.begin(), allowedVLANs.end());
    allowedVLANs.erase(std::unique(allowedVLANs.begin(), allowedVLANs.end()), allowedVLANs.end());
    switchportMode_ = SwitchportMode::Trunk;
    nativeVLAN_ = nativeVLAN;
    allowedVLANs_ = std::move(allowedVLANs);
    return true;
}

bool NetworkInterface::allowsVLAN(int vlanID) const noexcept {
    if (switchportMode_ == SwitchportMode::Access) return accessVLAN_ == vlanID;
    return std::find(allowedVLANs_.begin(), allowedVLANs_.end(), vlanID) != allowedVLANs_.end();
}

}  // namespace netlab
