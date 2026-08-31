#include "SimulationEngine.hpp"

#include "../Protocol/IPv4.hpp"

#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <utility>
#include <sstream>

namespace netlab {

bool SimulationEngine::addDevice(Device& device) {
    if (findDeviceWithIdentifier(device.identifier())) return false;
    devices_.push_back(&device);
    return true;
}

bool SimulationEngine::addLink(const Link& link) {
    if (!findDeviceWithIdentifier(link.firstEndpoint().deviceIdentifier) ||
        !findDeviceWithIdentifier(link.secondEndpoint().deviceIdentifier)) return false;
    for (const Link* existing : links_) {
        if (existing->identifier() == link.identifier()) return false;
        const auto endpointInUse = [&](const LinkEndpoint& endpoint) {
            return (existing->firstEndpoint().deviceIdentifier == endpoint.deviceIdentifier &&
                    existing->firstEndpoint().interfaceName == endpoint.interfaceName) ||
                   (existing->secondEndpoint().deviceIdentifier == endpoint.deviceIdentifier &&
                    existing->secondEndpoint().interfaceName == endpoint.interfaceName);
        };
        if (endpointInUse(link.firstEndpoint()) || endpointInUse(link.secondEndpoint())) return false;
    }
    links_.push_back(&link);
    return true;
}

Device* SimulationEngine::findDeviceWithAddress(const std::string& address) const noexcept {
    for (Device* device : devices_) {
        if (device->interfaceWithIPv4(address)) return device;
    }
    return nullptr;
}

Device* SimulationEngine::findDeviceWithIdentifier(const std::string& identifier) const noexcept {
    for (Device* device : devices_) if (device->identifier() == identifier) return device;
    return nullptr;
}

Device* SimulationEngine::findDefaultGateway(const Device& source) const noexcept {
    if (source.defaultGateway().empty()) return nullptr;
    return findDeviceWithAddress(source.defaultGateway());
}

std::string SimulationEngine::resolveDestination(Device& source, const std::string& addressOrName,
                                                  std::vector<SimulationEvent>& events) const {
    std::uint32_t ignored = 0;
    if (ipv4::parse(addressOrName, ignored)) return addressOrName;
    if (source.dnsServer().empty()) {
        events.push_back({"DNS", source.hostname() + " has no configured DNS server."});
        return {};
    }
    Device* server = findDeviceWithAddress(source.dnsServer());
    if (!server || !server->supportsCapability("DNS_SERVER")) {
        events.push_back({"DNS", "Configured DNS server " + source.dnsServer() + " is unavailable."});
        return {};
    }
    std::vector<SimulationEvent> reachabilityEvents;
    std::vector<PathHop> path = findLayer2Path(source, *server);
    int vlanID = 1;
    if (path.empty() || !validateVLANPath(path, vlanID, reachabilityEvents)) {
        events.push_back({"DNS", "Configured DNS server " + source.dnsServer() +
                         " is unreachable on the active VLAN topology."});
        return {};
    }
    auto record = server->dnsRecords().find(addressOrName);
    if (record != server->dnsRecords().end()) {
        events.push_back({"DNS", server->hostname() + " resolves " + addressOrName + " to " + record->second + "."});
        return record->second;
    }
    events.push_back({"DNS", server->hostname() + " has no record for " + addressOrName + "."});
    return {};
}

std::vector<SimulationEngine::PathHop> SimulationEngine::findLayer2Path(Device& source,
                                                                         Device& destination) const {
    struct Parent {
        Device* previous = nullptr;
        std::string incomingInterface;
        std::string previousOutgoingInterface;
    };
    std::queue<Device*> pending;
    std::set<Device*> visited;
    std::map<Device*, Parent> parents;
    pending.push(&source);
    visited.insert(&source);

    while (!pending.empty()) {
        Device* current = pending.front();
        pending.pop();
        if (!current) continue;
        if (current == &destination) break;
        if (current != &source && current->role() != DeviceRole::Switch) continue;
        for (const Link* link : links_) {
            if (!link || link->state() != LinkState::Up) continue;
            Device* firstDevice = findDeviceWithIdentifier(link->firstEndpoint().deviceIdentifier);
            Device* secondDevice = findDeviceWithIdentifier(link->secondEndpoint().deviceIdentifier);
            Device* neighbor = nullptr;
            std::string currentInterface;
            std::string neighborInterface;
            if (firstDevice == current) {
                neighbor = secondDevice;
                currentInterface = link->firstEndpoint().interfaceName;
                neighborInterface = link->secondEndpoint().interfaceName;
            } else if (secondDevice == current) {
                neighbor = firstDevice;
                currentInterface = link->secondEndpoint().interfaceName;
                neighborInterface = link->firstEndpoint().interfaceName;
            }
            if (!neighbor) continue;
            const NetworkInterface* currentPort = current->interfaceNamed(currentInterface);
            const NetworkInterface* neighborPort = neighbor->interfaceNamed(neighborInterface);
            if (!currentPort || !neighborPort || !currentPort->adminUp() || !neighborPort->adminUp()) continue;
            if (visited.count(neighbor)) continue;
            visited.insert(neighbor);
            parents[neighbor] = {current, neighborInterface, currentInterface};
            pending.push(neighbor);
        }
    }
    if (!visited.count(&destination)) return {};

    std::vector<Device*> reversedDevices;
    for (Device* current = &destination; current; current = parents[current].previous) {
        reversedDevices.push_back(current);
        if (current == &source) break;
    }
    std::reverse(reversedDevices.begin(), reversedDevices.end());

    std::vector<PathHop> path;
    path.reserve(reversedDevices.size());
    for (std::size_t index = 0; index < reversedDevices.size(); ++index) {
        Device* device = reversedDevices[index];
        std::string incoming = index == 0 ? std::string{} : parents[device].incomingInterface;
        std::string outgoing;
        if (index + 1 < reversedDevices.size()) outgoing = parents[reversedDevices[index + 1]].previousOutgoingInterface;
        path.push_back({device, std::move(incoming), std::move(outgoing)});
    }
    return path;
}

bool SimulationEngine::validateVLANPath(const std::vector<PathHop>& path, int& vlanID,
                                        std::vector<SimulationEvent>& events) const {
    vlanID = 1;
    for (const PathHop& hop : path) {
        if (hop.device->role() != DeviceRole::Switch) continue;
        const NetworkInterface* incoming = hop.device->interfaceNamed(hop.incomingInterface);
        if (incoming && incoming->switchportMode() == NetworkInterface::SwitchportMode::Access) {
            vlanID = incoming->accessVLAN();
            break;
        }
    }
    for (const PathHop& hop : path) {
        if (hop.device->role() != DeviceRole::Switch) continue;
        const NetworkInterface* incoming = hop.device->interfaceNamed(hop.incomingInterface);
        const NetworkInterface* outgoing = hop.device->interfaceNamed(hop.outgoingInterface);
        if (!incoming || !outgoing || !incoming->allowsVLAN(vlanID) || !outgoing->allowsVLAN(vlanID)) {
            events.push_back({"VLAN", hop.device->hostname() + " blocks VLAN " + std::to_string(vlanID) +
                              " between " + hop.incomingInterface + " and " + hop.outgoingInterface + "."});
            return false;
        }
        events.push_back({"VLAN", hop.device->hostname() + " forwards VLAN " + std::to_string(vlanID) +
                          (incoming->switchportMode() == NetworkInterface::SwitchportMode::Trunk ||
                           outgoing->switchportMode() == NetworkInterface::SwitchportMode::Trunk
                               ? " on an 802.1Q trunk." : " on access ports.")});
    }
    return true;
}

bool SimulationEngine::processLayer2Segment(Device& sender, Device& receiver,
                                            const std::string& arpAddress,
                                            std::vector<SimulationEvent>& events) {
    const NetworkInterface* senderInterface = sender.interfaceForSubnet(arpAddress);
    if (!senderInterface) senderInterface = sender.firstConfiguredInterface();
    NetworkInterface* receiverInterface = receiver.interfaceWithIPv4(arpAddress);
    if (!receiverInterface) {
        const NetworkInterface* matching = receiver.interfaceForSubnet(senderInterface ? senderInterface->ipv4Address() : "");
        receiverInterface = matching ? receiver.interfaceNamed(matching->name()) : nullptr;
    }
    if (!senderInterface || !receiverInterface) return false;
    std::vector<PathHop> path = findLayer2Path(sender, receiver);
    if (path.empty()) {
        events.push_back({"Ethernet", "No Layer 2 path connects " + sender.hostname() + " to " + receiver.hostname() + "."});
        return false;
    }
    int vlanID = 1;
    if (!validateVLANPath(path, vlanID, events)) return false;
    const std::string resolvedAddress = arpAddress.empty() ? receiverInterface->ipv4Address() : arpAddress;
    auto cached = sender.arpTable().find(resolvedAddress);
    if (cached == sender.arpTable().end()) {
        events.push_back({"ARP", sender.hostname() + " broadcasts: Who has " + resolvedAddress + "?"});
        events.push_back({"ARP", receiver.hostname() + " replies with " + receiverInterface->macAddress() + "."});
        sender.learnARP(resolvedAddress, receiverInterface->macAddress());
        receiver.learnARP(senderInterface->ipv4Address(), senderInterface->macAddress());
    } else {
        events.push_back({"ARP", "Cache hit: " + resolvedAddress + " is " + cached->second + "."});
    }
    for (const PathHop& hop : path) {
        if (hop.device->role() == DeviceRole::Switch) {
            hop.device->learnMAC(senderInterface->macAddress(), hop.incomingInterface);
            hop.device->learnMAC(receiverInterface->macAddress(), hop.outgoingInterface);
            events.push_back({"Switching", hop.device->hostname() + " forwards " + hop.incomingInterface +
                              " → " + hop.outgoingInterface + "."});
        }
    }
    return true;
}

PingResult SimulationEngine::ping(Device& source, const std::string& destinationAddress) {
    PingResult result;
    const std::string resolvedAddress = resolveDestination(source, destinationAddress, result.events);
    std::uint32_t ignored = 0;
    if (resolvedAddress.empty() || !ipv4::parse(resolvedAddress, ignored)) {
        result.summary = "Invalid IPv4 destination or DNS name not found.";
        return result;
    }
    const NetworkInterface* sourceInterface = source.firstConfiguredInterface();
    if (!sourceInterface) {
        result.summary = "Source device has no enabled IPv4 interface.";
        return result;
    }
    result.events.push_back({"Destination", "Target address " + resolvedAddress + " accepted."});

    Device* destination = findDeviceWithAddress(resolvedAddress);
    if (!destination) {
        result.events.push_back({"ARP", "No simulated device owns the destination address."});
        result.summary = "Destination host is not present in this topology.";
        return result;
    }
    NetworkInterface* destinationInterface = destination->interfaceWithIPv4(resolvedAddress);
    if (!destinationInterface || !destinationInterface->adminUp()) {
        result.summary = "Destination interface is down.";
        return result;
    }

    if (!ipv4::sameSubnet(sourceInterface->ipv4Address(), resolvedAddress,
                          sourceInterface->subnetMask())) {
        result.events.push_back({"Subnet", "Destination is outside the local subnet."});
        Device* router = findDefaultGateway(source);
        if (!router || (router->role() != DeviceRole::Router && router->role() != DeviceRole::Firewall)) {
            result.summary = "Destination is remote and no reachable default gateway is configured.";
            return result;
        }
        if (!processLayer2Segment(source, *router, source.defaultGateway(), result.events)) {
            result.summary = "Default gateway is unreachable at Layer 2 or blocked by VLAN policy.";
            return result;
        }
        std::optional<RouteEntry> route = router->bestRoute(resolvedAddress);
        if (!route) {
            result.events.push_back({"Routing", router->hostname() + " has no matching route."});
            result.summary = "Router has no route to the destination network.";
            return result;
        }
        result.events.push_back({"Routing", router->hostname() + " selects " + route->protocol + " route " +
                                 route->destination + " mask " + route->subnetMask + " via " +
                                 (route->nextHop.empty() ? route->interfaceName : route->nextHop) + "."});
        if (!router->permits(sourceInterface->ipv4Address(), resolvedAddress)) {
            result.events.push_back({router->firewallEnabled() ? "Firewall" : "ACL",
                                     "Policy denies this source/destination pair."});
            result.summary = "Packet denied by router security policy.";
            return result;
        }
        result.events.push_back({router->firewallEnabled() ? "Firewall" : "ACL", "Policy permits the ICMP flow."});
        if (router->natEnabled()) {
            router->addNATTranslation(sourceInterface->ipv4Address(), router->natPublicAddress());
            result.events.push_back({"NAT", sourceInterface->ipv4Address() + " translated to " +
                                     router->natPublicAddress() + "."});
        }
        if (!processLayer2Segment(*router, *destination, resolvedAddress, result.events)) {
            result.summary = "Destination network is disconnected or blocked by VLAN policy.";
            return result;
        }
        result.events.push_back({"IPv4", "Router decrements TTL from 64 to 63 and recalculates the header."});
    } else {
        result.events.push_back({"Subnet", "Destination is local; ARP resolution is required."});
        if (!processLayer2Segment(source, *destination, resolvedAddress, result.events)) {
            result.summary = "Destination is unreachable or isolated by VLAN.";
            return result;
        }
    }
    result.events.push_back({"ICMP", "Echo Request seq=1 sent to " + resolvedAddress + "."});
    result.events.push_back({"ICMP", "Echo Reply seq=1 returned to " + sourceInterface->ipv4Address() + "."});
    result.success = true;
    result.summary = "Reply from " + resolvedAddress + ": bytes=64 time<1ms TTL=" +
                     (ipv4::sameSubnet(sourceInterface->ipv4Address(), resolvedAddress, sourceInterface->subnetMask()) ? "64" : "63");
    return result;
}

ServiceResult SimulationEngine::requestDHCP(Device& client) {
    ServiceResult result;
    if (!client.supportsCapability("DHCP_CLIENT")) {
        result.summary = "This device does not support DHCP client operation.";
        return result;
    }
    NetworkInterface* clientInterface = client.interfaces().empty() ? nullptr : &client.interfaces().front();
    if (!clientInterface || !clientInterface->adminUp()) {
        result.summary = "Client has no enabled network interface.";
        return result;
    }
    result.events.push_back({"DHCP", "DHCPDISCOVER broadcast sent by " + client.hostname() + "."});
    Device* server = nullptr;
    for (Device* candidate : devices_) {
        if (!candidate->dhcpServerEnabled()) continue;
        std::vector<PathHop> path = findLayer2Path(client, *candidate);
        std::vector<SimulationEvent> vlanEvents;
        int vlanID = 1;
        if (path.empty() || !validateVLANPath(path, vlanID, vlanEvents)) continue;
        server = candidate;
        result.events.insert(result.events.end(), vlanEvents.begin(), vlanEvents.end());
        result.events.push_back({"DHCP", candidate->hostname() +
                                 " receives the broadcast on VLAN " + std::to_string(vlanID) + "."});
        break;
    }
    if (!server) {
        result.summary = "No reachable DHCP server is available on this VLAN.";
        return result;
    }
    std::uint32_t configuredNetwork = 0, mask = 0;
    if (!ipv4::parse(server->dhcpNetwork(), configuredNetwork) ||
        !ipv4::parse(server->dhcpSubnetMask(), mask) ||
        !ipv4::isValidSubnetMask(server->dhcpSubnetMask())) {
        result.summary = "DHCP pool network or subnet mask is invalid.";
        return result;
    }
    const std::uint32_t network = configuredNetwork & mask;
    const std::uint32_t broadcast = network | ~mask;
    if (static_cast<std::uint64_t>(network) + 1 >= broadcast) {
        result.summary = "DHCP pool has no usable host addresses.";
        return result;
    }

    auto existingLease = server->dhcpLeases().find(client.identifier());
    std::uint32_t addressValue = 0;
    if (existingLease != server->dhcpLeases().end()) {
        ipv4::parse(existingLease->second, addressValue);
    } else {
        std::set<std::uint32_t> usedAddresses;
        for (Device* device : devices_) {
            for (const auto& networkInterface : device->interfaces()) {
                std::uint32_t used = 0;
                if (ipv4::parse(networkInterface.ipv4Address(), used)) usedAddresses.insert(used);
            }
        }
        for (const auto& lease : server->dhcpLeases()) {
            std::uint32_t used = 0;
            if (ipv4::parse(lease.second, used)) usedAddresses.insert(used);
        }
        std::uint32_t gateway = 0;
        if (ipv4::parse(server->dhcpGateway(), gateway)) usedAddresses.insert(gateway);

        const std::uint64_t firstUsable = static_cast<std::uint64_t>(network) + 1;
        const std::uint64_t lastUsable = static_cast<std::uint64_t>(broadcast) - 1;
        std::uint64_t preferred = static_cast<std::uint64_t>(network) + 100;
        if (preferred < firstUsable || preferred > lastUsable) preferred = firstUsable;
        for (std::uint64_t candidate = preferred; candidate <= lastUsable; ++candidate) {
            if (!usedAddresses.count(static_cast<std::uint32_t>(candidate))) {
                addressValue = static_cast<std::uint32_t>(candidate);
                break;
            }
        }
        if (addressValue == 0 && preferred != firstUsable) {
            for (std::uint64_t candidate = firstUsable; candidate < preferred; ++candidate) {
                if (!usedAddresses.count(static_cast<std::uint32_t>(candidate))) {
                    addressValue = static_cast<std::uint32_t>(candidate);
                    break;
                }
            }
        }
        if (addressValue == 0) {
            result.summary = "DHCP address pool is exhausted.";
            return result;
        }
    }

    const std::string address = ipv4::format(addressValue);
    if (!client.applyDHCPLeaseConfiguration(clientInterface->name(), address,
                                             server->dhcpSubnetMask(), server->dhcpGateway(),
                                             server->dhcpDNSServer())) {
        result.summary = "DHCP offer contained invalid configuration.";
        return result;
    }
    server->addDHCPLease(client.identifier(), address);
    result.events.push_back({"DHCP", server->hostname() + " sends DHCPOFFER " + address + "."});
    result.events.push_back({"DHCP", "DHCPREQUEST / DHCPACK completed; gateway=" + server->dhcpGateway() +
                             " DNS=" + server->dhcpDNSServer() + "."});
    result.success = true;
    result.summary = "Lease applied: " + address + " / " + server->dhcpSubnetMask();
    return result;
}

std::string SimulationEngine::advancedStatus(const Device& device) const {
    std::ostringstream output;
    output << "Milestone 7 learning state\n";
    output << "STP: " << (device.stpEnabled() ? "Forwarding (loop prevention active)" : "Disabled") << '\n';
    output << "Dynamic routing: " << (device.dynamicRoutingProtocol().empty() ? "Disabled" : device.dynamicRoutingProtocol() + " converged") << '\n';
    const NetworkInterface* networkInterface = device.firstConfiguredInterface();
    output << "IPv6: " << (networkInterface && !networkInterface->ipv6Address().empty()
        ? networkInterface->ipv6Address() + "/" + std::to_string(networkInterface->ipv6PrefixLength()) : "Not configured") << '\n';
    output << "Wireless: " << (device.wirelessSSID().empty() ? "Not configured" :
        "SSID " + device.wirelessSSID() + (device.wirelessSecured() ? " (secured)" : " (open)")) << '\n';
    output << "Firewall: " << (device.firewallEnabled() ? "Stateful policy enabled" : "Disabled") << '\n';
    output << "VPN: " << (device.vpnUp() ? "Tunnel up to " + device.vpnPeer() : "No active tunnel") << '\n';
    return output.str();
}

}  // namespace netlab
