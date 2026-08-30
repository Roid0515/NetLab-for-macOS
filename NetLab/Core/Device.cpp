#include "Device.hpp"

#include "../Protocol/IPv4.hpp"

#include <iomanip>
#include <algorithm>
#include <sstream>
#include <utility>

namespace netlab {

Device::Device(std::string identifier,
               std::string hostname,
               DeviceRole role,
               const std::vector<std::pair<std::string, int>>& interfaces)
    : identifier_(std::move(identifier)), hostname_(std::move(hostname)), role_(role) {
    interfaces_.reserve(interfaces.size());
    for (std::size_t index = 0; index < interfaces.size(); ++index) {
        interfaces_.emplace_back(interfaces[index].first, generateMAC(identifier_, index), interfaces[index].second);
    }
}

bool Device::setDefaultGateway(const std::string& address) {
    if (address.empty()) {
        defaultGateway_.clear();
        return true;
    }
    std::uint32_t ignored = 0;
    if (!ipv4::parse(address, ignored)) return false;
    defaultGateway_ = address;
    return true;
}

NetworkInterface* Device::interfaceNamed(const std::string& name) noexcept {
    for (auto& networkInterface : interfaces_) {
        if (networkInterface.name() == name) return &networkInterface;
    }
    return nullptr;
}

const NetworkInterface* Device::interfaceNamed(const std::string& name) const noexcept {
    for (const auto& networkInterface : interfaces_) {
        if (networkInterface.name() == name) return &networkInterface;
    }
    return nullptr;
}

NetworkInterface* Device::interfaceWithIPv4(const std::string& address) noexcept {
    for (auto& networkInterface : interfaces_) {
        if (networkInterface.ipv4Address() == address) return &networkInterface;
    }
    return nullptr;
}

const NetworkInterface* Device::firstConfiguredInterface() const noexcept {
    for (const auto& networkInterface : interfaces_) {
        if (networkInterface.hasIPv4Configuration() && networkInterface.adminUp()) return &networkInterface;
    }
    return nullptr;
}

const NetworkInterface* Device::interfaceForSubnet(const std::string& address) const noexcept {
    for (const auto& networkInterface : interfaces_) {
        if (networkInterface.hasIPv4Configuration() && networkInterface.adminUp() &&
            ipv4::sameSubnet(networkInterface.ipv4Address(), address, networkInterface.subnetMask())) {
            return &networkInterface;
        }
    }
    return nullptr;
}

void Device::learnARP(const std::string& address, const std::string& macAddress) {
    arpTable_[address] = macAddress;
}

void Device::learnMAC(const std::string& macAddress, const std::string& interfaceName) {
    macAddressTable_[macAddress] = interfaceName;
}

bool Device::addStaticRoute(const std::string& destination, const std::string& subnetMask,
                            const std::string& nextHop, const std::string& interfaceName) {
    std::uint32_t ignored = 0;
    if (!ipv4::parse(destination, ignored) ||
        (subnetMask != "0.0.0.0" && !ipv4::isValidSubnetMask(subnetMask)) ||
        (!nextHop.empty() && !ipv4::parse(nextHop, ignored))) return false;
    staticRoutes_.push_back({destination, subnetMask, nextHop, interfaceName, 1,
                             destination == "0.0.0.0" ? "DEFAULT" : "STATIC"});
    return true;
}

std::vector<RouteEntry> Device::routingTable() const {
    std::vector<RouteEntry> routes;
    for (const auto& networkInterface : interfaces_) {
        if (!networkInterface.hasIPv4Configuration()) continue;
        std::uint32_t address = 0, mask = 0;
        if (!ipv4::parse(networkInterface.ipv4Address(), address) ||
            !ipv4::parse(networkInterface.subnetMask(), mask)) continue;
        std::uint32_t network = address & mask;
        std::ostringstream destination;
        destination << ((network >> 24) & 0xff) << '.' << ((network >> 16) & 0xff) << '.'
                    << ((network >> 8) & 0xff) << '.' << (network & 0xff);
        routes.push_back({destination.str(), networkInterface.subnetMask(), {},
                          networkInterface.name(), 0, "CONNECTED"});
    }
    routes.insert(routes.end(), staticRoutes_.begin(), staticRoutes_.end());
    return routes;
}

std::optional<RouteEntry> Device::bestRoute(const std::string& destination) const {
    std::uint32_t target = 0;
    if (!ipv4::parse(destination, target)) return std::nullopt;
    std::optional<RouteEntry> best;
    int bestBits = -1;
    for (const auto& route : routingTable()) {
        std::uint32_t network = 0, mask = 0;
        if (!ipv4::parse(route.destination, network) || !ipv4::parse(route.subnetMask, mask)) continue;
        if ((target & mask) != (network & mask)) continue;
        int bits = 0;
        for (std::uint32_t value = mask; value; value >>= 1) bits += value & 1U;
        if (bits > bestBits || (bits == bestBits && best && route.metric < best->metric)) {
            best = route;
            bestBits = bits;
        }
    }
    return best;
}

void Device::setDHCPServer(bool enabled, std::string network, std::string subnetMask,
                           std::string gateway, std::string dnsServer) {
    dhcpServerEnabled_ = enabled;
    dhcpNetwork_ = std::move(network);
    dhcpSubnetMask_ = std::move(subnetMask);
    dhcpGateway_ = std::move(gateway);
    dhcpDNSServer_ = std::move(dnsServer);
}

void Device::addDHCPLease(const std::string& client, const std::string& address) {
    dhcpLeases_[client] = address;
}

void Device::addDNSRecord(const std::string& name, const std::string& address) {
    dnsRecords_[name] = address;
}

void Device::setNATEnabled(bool enabled, std::string publicAddress) {
    natEnabled_ = enabled;
    natPublicAddress_ = std::move(publicAddress);
}

void Device::addNATTranslation(std::string insideLocal, std::string insideGlobal) {
    natTranslations_.push_back({std::move(insideLocal), std::move(insideGlobal)});
}

void Device::addACLRule(ACLRule rule) {
    aclRules_.push_back(std::move(rule));
}

bool Device::permits(const std::string& source, const std::string& destination) const noexcept {
    for (const auto& rule : aclRules_) {
        const bool sourceMatches = rule.source == "any" || rule.source == source;
        const bool destinationMatches = rule.destination == "any" || rule.destination == destination;
        if (sourceMatches && destinationMatches) return rule.permit;
    }
    return true;
}

void Device::setWireless(std::string ssid, bool secured) {
    wirelessSSID_ = std::move(ssid);
    wirelessSecured_ = secured;
}

void Device::setVPNTunnel(std::string peer, bool up) {
    vpnPeer_ = std::move(peer);
    vpnUp_ = up;
}

std::string Device::generateMAC(const std::string& identifier, std::size_t interfaceIndex) {
    std::uint64_t hash = 1469598103934665603ULL;
    for (unsigned char character : identifier) {
        hash ^= character;
        hash *= 1099511628211ULL;
    }
    hash ^= interfaceIndex + 1;
    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::nouppercase
           << std::setw(2) << 0x02 << ':'
           << std::setw(2) << ((hash >> 32) & 0xff) << ':'
           << std::setw(2) << ((hash >> 24) & 0xff) << ':'
           << std::setw(2) << ((hash >> 16) & 0xff) << ':'
           << std::setw(2) << ((hash >> 8) & 0xff) << ':'
           << std::setw(2) << (hash & 0xff);
    return stream.str();
}

}  // namespace netlab
