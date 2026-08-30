#pragma once

#include "NetworkInterface.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace netlab {

enum class DeviceRole { Endpoint, Switch, Router, Server, Firewall, WirelessAP };

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

class Device final {
public:
    Device(std::string identifier,
           std::string hostname,
           DeviceRole role,
           const std::vector<std::pair<std::string, int>>& interfaces);

    const std::string& identifier() const noexcept { return identifier_; }
    const std::string& hostname() const noexcept { return hostname_; }
    DeviceRole role() const noexcept { return role_; }
    const std::string& defaultGateway() const noexcept { return defaultGateway_; }
    bool setDefaultGateway(const std::string& address);

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

    void setDHCPServer(bool enabled, std::string network = {}, std::string subnetMask = {},
                       std::string gateway = {}, std::string dnsServer = {});
    bool dhcpServerEnabled() const noexcept { return dhcpServerEnabled_; }
    const std::string& dhcpNetwork() const noexcept { return dhcpNetwork_; }
    const std::string& dhcpSubnetMask() const noexcept { return dhcpSubnetMask_; }
    const std::string& dhcpGateway() const noexcept { return dhcpGateway_; }
    const std::string& dhcpDNSServer() const noexcept { return dhcpDNSServer_; }
    void addDHCPLease(const std::string& client, const std::string& address);
    const std::map<std::string, std::string>& dhcpLeases() const noexcept { return dhcpLeases_; }
    void addDNSRecord(const std::string& name, const std::string& address);
    const std::map<std::string, std::string>& dnsRecords() const noexcept { return dnsRecords_; }
    void setNATEnabled(bool enabled, std::string publicAddress = {});
    bool natEnabled() const noexcept { return natEnabled_; }
    const std::string& natPublicAddress() const noexcept { return natPublicAddress_; }
    void addNATTranslation(std::string insideLocal, std::string insideGlobal);
    const std::vector<NATTranslation>& natTranslations() const noexcept { return natTranslations_; }
    void addACLRule(ACLRule rule);
    const std::vector<ACLRule>& aclRules() const noexcept { return aclRules_; }
    bool permits(const std::string& source, const std::string& destination) const noexcept;

    void enableSTP(bool enabled) noexcept { stpEnabled_ = enabled; }
    bool stpEnabled() const noexcept { return stpEnabled_; }
    void setDynamicRoutingProtocol(std::string protocol) { dynamicRoutingProtocol_ = std::move(protocol); }
    const std::string& dynamicRoutingProtocol() const noexcept { return dynamicRoutingProtocol_; }
    void setWireless(std::string ssid, bool secured);
    const std::string& wirelessSSID() const noexcept { return wirelessSSID_; }
    bool wirelessSecured() const noexcept { return wirelessSecured_; }
    void setFirewallEnabled(bool enabled) noexcept { firewallEnabled_ = enabled; }
    bool firewallEnabled() const noexcept { return firewallEnabled_; }
    void setVPNTunnel(std::string peer, bool up);
    const std::string& vpnPeer() const noexcept { return vpnPeer_; }
    bool vpnUp() const noexcept { return vpnUp_; }

private:
    static std::string generateMAC(const std::string& identifier, std::size_t interfaceIndex);

    std::string identifier_;
    std::string hostname_;
    DeviceRole role_;
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
