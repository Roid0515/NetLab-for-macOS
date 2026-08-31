#pragma once

#include "DeviceDefinition.hpp"
#include "NetworkInterface.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace netlab {

struct RouteEntry {
    std::string destination;
    std::string subnetMask;
    std::string nextHop;
    std::string interfaceName;
    int metric = 1;
    std::string protocol = "STATIC";
};

struct ACLRule {
    bool permit = true;
    std::string source = "any";
    std::string destination = "any";
    std::string description;
};

struct NATTranslation {
    std::string insideLocal;
    std::string insideGlobal;
};

struct InterfaceConfiguration {
    std::string interfaceName;
    std::string ipv4Address;
    std::string subnetMask;
    std::string defaultGateway;
    std::string ipv6Address;
    int ipv6PrefixLength = 64;
    NetworkInterface::SwitchportMode switchportMode = NetworkInterface::SwitchportMode::Access;
    int vlanID = 1;
};

class Device final {
public:
    Device(std::string identifier,
           std::string hostname,
           DeviceRole role,
           const std::vector<std::pair<std::string, int>>& interfaces,
           std::vector<std::string> capabilities = {});

    const std::string& identifier() const noexcept { return identifier_; }
    const std::string& hostname() const noexcept { return hostname_; }
    DeviceRole role() const noexcept { return role_; }
    bool supportsCapability(const std::string& capability) const noexcept;
    const std::string& defaultGateway() const noexcept { return defaultGateway_; }
    bool setDefaultGateway(const std::string& address);
    bool applyInterfaceConfiguration(const InterfaceConfiguration& configuration);

    std::vector<NetworkInterface>& interfaces() noexcept { return interfaces_; }
    const std::vector<NetworkInterface>& interfaces() const noexcept { return interfaces_; }
    NetworkInterface* interfaceNamed(const std::string& name) noexcept;
    const NetworkInterface* interfaceNamed(const std::string& name) const noexcept;
    NetworkInterface* interfaceWithIPv4(const std::string& address) noexcept;
    const NetworkInterface* firstConfiguredInterface() const noexcept;
    const NetworkInterface* interfaceForSubnet(const std::string& address) const noexcept;

    void learnARP(const std::string& address, const std::string& macAddress);
    const std::map<std::string, std::string>& arpTable() const noexcept { return arpTable_; }
    void learnMAC(const std::string& macAddress, const std::string& interfaceName);
    const std::map<std::string, std::string>& macAddressTable() const noexcept { return macAddressTable_; }

    bool addStaticRoute(const std::string& destination, const std::string& subnetMask,
                        const std::string& nextHop, const std::string& interfaceName = {});
    std::vector<RouteEntry> routingTable() const;
    std::optional<RouteEntry> bestRoute(const std::string& destination) const;

    bool setDHCPServer(bool enabled, std::string network = {}, std::string subnetMask = {},
                       std::string gateway = {}, std::string dnsServer = {});
    bool dhcpServerEnabled() const noexcept { return dhcpServerEnabled_; }
    const std::string& dhcpNetwork() const noexcept { return dhcpNetwork_; }
    const std::string& dhcpSubnetMask() const noexcept { return dhcpSubnetMask_; }
    const std::string& dhcpGateway() const noexcept { return dhcpGateway_; }
    const std::string& dhcpDNSServer() const noexcept { return dhcpDNSServer_; }
    bool applyDHCPLeaseConfiguration(const std::string& interfaceName,
                                     const std::string& address,
                                     const std::string& subnetMask,
                                     const std::string& gateway,
                                     const std::string& dnsServer);
    bool addDHCPLease(const std::string& client, const std::string& address);
    const std::map<std::string, std::string>& dhcpLeases() const noexcept { return dhcpLeases_; }
    bool setDNSServer(std::string address);
    const std::string& dnsServer() const noexcept { return dnsServer_; }
    bool addDNSRecord(const std::string& name, const std::string& address);
    const std::map<std::string, std::string>& dnsRecords() const noexcept { return dnsRecords_; }
    bool setNATEnabled(bool enabled, std::string publicAddress = {});
    bool natEnabled() const noexcept { return natEnabled_; }
    const std::string& natPublicAddress() const noexcept { return natPublicAddress_; }
    bool addNATTranslation(std::string insideLocal, std::string insideGlobal);
    const std::vector<NATTranslation>& natTranslations() const noexcept { return natTranslations_; }
    bool addACLRule(ACLRule rule);
    const std::vector<ACLRule>& aclRules() const noexcept { return aclRules_; }
    bool permits(const std::string& source, const std::string& destination) const noexcept;

    bool enableSTP(bool enabled) noexcept;
    bool stpEnabled() const noexcept { return stpEnabled_; }
    bool setDynamicRoutingProtocol(std::string protocol);
    const std::string& dynamicRoutingProtocol() const noexcept { return dynamicRoutingProtocol_; }
    bool setWireless(std::string ssid, bool secured);
    const std::string& wirelessSSID() const noexcept { return wirelessSSID_; }
    bool wirelessSecured() const noexcept { return wirelessSecured_; }
    bool setFirewallEnabled(bool enabled) noexcept;
    bool firewallEnabled() const noexcept { return firewallEnabled_; }
    bool setVPNTunnel(std::string peer, bool up);
    const std::string& vpnPeer() const noexcept { return vpnPeer_; }
    bool vpnUp() const noexcept { return vpnUp_; }

private:
    static std::string generateMAC(const std::string& identifier, std::size_t interfaceIndex);

    std::string identifier_;
    std::string hostname_;
    DeviceRole role_;
    std::vector<std::string> capabilities_;
    std::vector<NetworkInterface> interfaces_;
    std::string defaultGateway_;
    std::map<std::string, std::string> arpTable_;
    std::map<std::string, std::string> macAddressTable_;
    std::vector<RouteEntry> staticRoutes_;
    bool dhcpServerEnabled_ = false;
    std::string dhcpNetwork_;
    std::string dhcpSubnetMask_;
    std::string dhcpGateway_;
    std::string dhcpDNSServer_;
    std::map<std::string, std::string> dhcpLeases_;
    std::string dnsServer_;
    std::map<std::string, std::string> dnsRecords_;
    bool natEnabled_ = false;
    std::string natPublicAddress_;
    std::vector<NATTranslation> natTranslations_;
    std::vector<ACLRule> aclRules_;
    bool stpEnabled_ = false;
    std::string dynamicRoutingProtocol_;
    std::string wirelessSSID_;
    bool wirelessSecured_ = false;
    bool firewallEnabled_ = false;
    std::string vpnPeer_;
    bool vpnUp_ = false;
};

}  // namespace netlab
