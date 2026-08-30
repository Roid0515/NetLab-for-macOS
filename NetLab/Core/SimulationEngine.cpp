#include "SimulationEngine.hpp"

#include "../Protocol/IPv4.hpp"

#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <utility>
#include <sstream>

namespace netlab {

void SimulationEngine::addDevice(Device& device) {
    devices_.push_back(&device);
}

void SimulationEngine::addEthernetConnection(EthernetConnection connection) {
    connections_.push_back(std::move(connection));
}

Device* SimulationEngine::findDeviceWithAddress(const std::string& address) const noexcept {
    for (Device* device : devices_) {
        if (device->interfaceWithIPv4(address)) return device;
    }
    return nullptr;
}

Device* SimulationEngine::findDefaultGateway(const Device& source) const noexcept {
    if (source.defaultGateway().empty()) return nullptr;
    return findDeviceWithAddress(source.defaultGateway());
}

std::string SimulationEngine::resolveDestination(const std::string& addressOrName,
                                                  std::vector<SimulationEvent>& events) const {
    std::uint32_t ignored = 0;
    if (ipv4::parse(addressOrName, ignored)) return addressOrName;
    for (Device* device : devices_) {
        auto record = device->dnsRecords().find(addressOrName);
        if (record != device->dnsRecords().end()) {
            events.push_back({"DNS", device->hostname() + " resolves " + addressOrName + " to " + record->second + "."});
            return record->second;
        }
    }
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
        if (current == &destination) break;
        if (current != &source && current->role() != DeviceRole::Switch) continue;
        for (const auto& connection : connections_) {
            Device* neighbor = nullptr;
            std::string currentInterface;
            std::string neighborInterface;
            if (connection.firstDevice == current) {
                neighbor = connection.secondDevice;
                currentInterface = connection.firstInterface;
                neighborInterface = connection.secondInterface;
            } else if (connection.secondDevice == current) {
                neighbor = connection.firstDevice;
                currentInterface = connection.secondInterface;
                neighborInterface = connection.firstInterface;
            }
            if (!neighbor || visited.count(neighbor)) continue;
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
    const std::string resolvedAddress = resolveDestination(destinationAddress, result.events);
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
    NetworkInterface* clientInterface = client.interfaces().empty() ? nullptr : &client.interfaces().front();
    if (!clientInterface) {
        result.summary = "Client has no network interface.";
        return result;
    }
    result.events.push_back({"DHCP", "DHCPDISCOVER broadcast sent by " + client.hostname() + "."});
    Device* server = nullptr;
    for (Device* candidate : devices_) if (candidate->dhcpServerEnabled()) { server = candidate; break; }
    if (!server) {
        result.summary = "No DHCP server is configured.";
        return result;
    }
    std::uint32_t network = 0;
    if (!ipv4::parse(server->dhcpNetwork(), network)) {
        result.summary = "DHCP pool network is invalid.";
        return result;
    }
    const std::uint32_t host = 100 + static_cast<std::uint32_t>(server->dhcpLeases().size());
    const std::uint32_t addressValue = (network & 0xffffff00U) | (host & 0xffU);
    std::ostringstream address;
    address << ((addressValue >> 24) & 0xff) << '.' << ((addressValue >> 16) & 0xff) << '.'
            << ((addressValue >> 8) & 0xff) << '.' << (addressValue & 0xff);
    if (!clientInterface->configureIPv4(address.str(), server->dhcpSubnetMask()) ||
        !client.setDefaultGateway(server->dhcpGateway())) {
        result.summary = "DHCP offer contained invalid configuration.";
        return result;
    }
    server->addDHCPLease(client.identifier(), address.str());
    result.events.push_back({"DHCP", server->hostname() + " sends DHCPOFFER " + address.str() + "."});
    result.events.push_back({"DHCP", "DHCPREQUEST / DHCPACK completed; gateway=" + server->dhcpGateway() +
                             " DNS=" + server->dhcpDNSServer() + "."});
    result.success = true;
    result.summary = "Lease applied: " + address.str() + " / " + server->dhcpSubnetMask();
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
