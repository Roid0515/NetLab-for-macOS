#include "Device.hpp"

#include "../Protocol/IPv4.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <utility>

namespace netlab {

namespace {

bool isValidHostName(const std::string& name) noexcept {
    if (name.empty() || name.size() > 253 || name.front() == '.' || name.back() == '.') return false;
    for (unsigned char character : name) {
        if (!(std::isalnum(character) || character == '-' || character == '.')) return false;
    }
    return true;
}

bool isIPv4OrAny(const std::string& value) noexcept {
    if (value == "any") return true;
    std::uint32_t ignored = 0;
    return ipv4::parse(value, ignored);
}

}  // namespace

Device::Device(std::string identifier,
               std::string hostname,
               DeviceRole role,
               const std::vector<std::pair<std::string, int>>& interfaces,
               std::vector<std::string> capabilities)
    : identifier_(std::move(identifier)), hostname_(std::move(hostname)), role_(role),
      capabilities_(std::move(capabilities)) {
    interfaces_.reserve(interfaces.size());
    for (std::size_t index = 0; index < interfaces.size(); ++index) {
        interfaces_.emplace_back(interfaces[index].first, generateMAC(identifier_, index), interfaces[index].second);
    }
}

bool Device::supportsCapability(const std::string& capability) const noexcept {
    return std::find(capabilities_.begin(), capabilities_.end(), capability) != capabilities_.end();
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

bool Device::applyInterfaceConfiguration(const InterfaceConfiguration& configuration) {
    NetworkInterface* networkInterface = interfaceNamed(configuration.interfaceName);
    if (!networkInterface) return false;

    NetworkInterface candidate = *networkInterface;
    if (!candidate.configureIPv4(configuration.ipv4Address, configuration.subnetMask)) return false;
    if (!configuration.defaultGateway.empty()) {
        std::uint32_t ignored = 0;
        if (!ipv4::parse(configuration.defaultGateway, ignored)) return false;
    }
    if (configuration.ipv6Address.empty()) {
        candidate.clearIPv6Configuration();
    } else if (!candidate.configureIPv6(configuration.ipv6Address,
                                         configuration.ipv6PrefixLength)) {
        return false;
    }
    const bool vlanValid = configuration.switchportMode == NetworkInterface::SwitchportMode::Trunk
        ? candidate.configureTrunk(configuration.vlanID, {configuration.vlanID})
        : candidate.configureAccessVLAN(configuration.vlanID);
    if (!vlanValid) return false;

    *networkInterface = std::move(candidate);
    defaultGateway_ = configuration.defaultGateway;
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
    if (!supportsCapability("L3_ROUTING") || !ipv4::parse(destination, ignored) ||
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
        routes.push_back({ipv4::format(network), networkInterface.subnetMask(), {},
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

bool Device::setDHCPServer(bool enabled, std::string network, std::string subnetMask,
                           std::string gateway, std::string dnsServer) {
    if (!supportsCapability("DHCP_SERVER")) return false;
    if (enabled) {
        std::uint32_t networkValue = 0, maskValue = 0, ignored = 0;
        if (!ipv4::parse(network, networkValue) || !ipv4::parse(subnetMask, maskValue) ||
            !ipv4::isValidSubnetMask(subnetMask) ||
            (!gateway.empty() && !ipv4::parse(gateway, ignored)) ||
            (!dnsServer.empty() && !ipv4::parse(dnsServer, ignored))) return false;
        network = ipv4::format(networkValue & maskValue);
    }
    dhcpServerEnabled_ = enabled;
    dhcpNetwork_ = std::move(network);
    dhcpSubnetMask_ = std::move(subnetMask);
    dhcpGateway_ = std::move(gateway);
    dhcpDNSServer_ = std::move(dnsServer);
    if (!enabled) dhcpLeases_.clear();
    return true;
}

bool Device::applyDHCPLeaseConfiguration(const std::string& interfaceName,
                                         const std::string& address,
                                         const std::string& subnetMask,
                                         const std::string& gateway,
                                         const std::string& dnsServer) {
    NetworkInterface* networkInterface = interfaceNamed(interfaceName);
    if (!networkInterface) return false;
    NetworkInterface candidate = *networkInterface;
    std::uint32_t ignored = 0;
    if (!candidate.configureIPv4(address, subnetMask) ||
        (!gateway.empty() && !ipv4::parse(gateway, ignored)) ||
        (!dnsServer.empty() && !ipv4::parse(dnsServer, ignored))) return false;
    *networkInterface = std::move(candidate);
    defaultGateway_ = gateway;
    dnsServer_ = dnsServer;
    return true;
}

bool Device::addDHCPLease(const std::string& client, const std::string& address) {
    std::uint32_t ignored = 0;
    if (!dhcpServerEnabled_ || client.empty() || !ipv4::parse(address, ignored)) return false;
    dhcpLeases_[client] = address;
    return true;
}

bool Device::setDNSServer(std::string address) {
    std::uint32_t ignored = 0;
    if (!address.empty() && !ipv4::parse(address, ignored)) return false;
    dnsServer_ = std::move(address);
    return true;
}

bool Device::addDNSRecord(const std::string& name, const std::string& address) {
    std::uint32_t ignored = 0;
    if (!supportsCapability("DNS_SERVER") || !isValidHostName(name) ||
        !ipv4::parse(address, ignored)) return false;
    dnsRecords_[name] = address;
    return true;
}

bool Device::setNATEnabled(bool enabled, std::string publicAddress) {
    std::uint32_t ignored = 0;
    if (!supportsCapability("NAT") ||
        (enabled && !ipv4::parse(publicAddress, ignored))) return false;
    natEnabled_ = enabled;
    natPublicAddress_ = std::move(publicAddress);
    if (!enabled) natTranslations_.clear();
    return true;
}

bool Device::addNATTranslation(std::string insideLocal, std::string insideGlobal) {
    std::uint32_t ignored = 0;
    if (!natEnabled_ || !ipv4::parse(insideLocal, ignored) ||
        !ipv4::parse(insideGlobal, ignored)) return false;
    natTranslations_.push_back({std::move(insideLocal), std::move(insideGlobal)});
    return true;
}

bool Device::addACLRule(ACLRule rule) {
    if (!supportsCapability("ACL") || !isIPv4OrAny(rule.source) ||
        !isIPv4OrAny(rule.destination)) return false;
    aclRules_.push_back(std::move(rule));
    return true;
}

bool Device::permits(const std::string& source, const std::string& destination) const noexcept {
    for (const auto& rule : aclRules_) {
        const bool sourceMatches = rule.source == "any" || rule.source == source;
        const bool destinationMatches = rule.destination == "any" || rule.destination == destination;
        if (sourceMatches && destinationMatches) return rule.permit;
    }
    return true;
}

bool Device::enableSTP(bool enabled) noexcept {
    if (!supportsCapability("STP")) return false;
    stpEnabled_ = enabled;
    return true;
}

bool Device::setDynamicRoutingProtocol(std::string protocol) {
    if (!supportsCapability("L3_ROUTING")) return false;
    dynamicRoutingProtocol_ = std::move(protocol);
    return true;
}

bool Device::setWireless(std::string ssid, bool secured) {
    if (!supportsCapability("WIRELESS") || ssid.empty()) return false;
    wirelessSSID_ = std::move(ssid);
    wirelessSecured_ = secured;
    return true;
}

bool Device::setFirewallEnabled(bool enabled) noexcept {
    if (!supportsCapability("FIREWALL")) return false;
    firewallEnabled_ = enabled;
    return true;
}

bool Device::setVPNTunnel(std::string peer, bool up) {
    if (!supportsCapability("VPN") || (up && peer.empty())) return false;
    vpnPeer_ = std::move(peer);
    vpnUp_ = up;
    return true;
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
