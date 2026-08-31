#include "../NetLab/Core/Device.hpp"
#include "../NetLab/Core/DeviceCatalog.hpp"
#include "../NetLab/Core/Link.hpp"
#include "../NetLab/Core/SimulationEngine.hpp"
#include "../NetLab/Protocol/IPv4.hpp"

#include <cstdlib>
#include <iostream>
#include <set>
#include <string>

extern "C" bool NLRunUISmokeTests(void);

namespace {

int failures = 0;
int checks = 0;

void expect(bool condition, const std::string& message) {
    ++checks;
    if (condition) return;
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
}

netlab::Device endpoint(std::string identifier, std::string hostname) {
    return netlab::Device(std::move(identifier), std::move(hostname),
                          netlab::DeviceRole::Endpoint, {{"G0", 1000}},
                          {"DHCP_CLIENT"});
}

netlab::Device server(std::string identifier, std::string hostname) {
    return netlab::Device(std::move(identifier), std::move(hostname),
                          netlab::DeviceRole::Server, {{"G0", 1000}},
                          {"DHCP_SERVER", "DNS_SERVER"});
}

netlab::Link activeLink(const std::string& identifier,
                        const netlab::Device& first, const std::string& firstInterface,
                        const netlab::Device& second, const std::string& secondInterface) {
    netlab::Link link(identifier,
                      {first.identifier(), firstInterface},
                      {second.identifier(), secondInterface}, 1000);
    link.setState(netlab::LinkState::Up);
    return link;
}

void testIPv4AndIPv6() {
    std::uint32_t value = 0;
    expect(netlab::ipv4::parse("192.168.10.7", value), "valid IPv4 parses");
    expect(netlab::ipv4::format(value) == "192.168.10.7", "IPv4 format round trip");
    expect(!netlab::ipv4::parse("192.168.1.999", value), "invalid IPv4 rejected");
    expect(netlab::ipv4::isValidSubnetMask("255.255.255.128"), "valid /25 mask");
    expect(!netlab::ipv4::isValidSubnetMask("255.0.255.0"), "non-contiguous mask rejected");
    expect(netlab::ipv4::sameSubnet("10.0.1.10", "10.0.1.250", "255.255.255.0"),
           "same subnet accepted");

    netlab::NetworkInterface networkInterface("G0", "02:00:00:00:00:01", 1000);
    expect(networkInterface.configureIPv6("2001:db8::1", 0), "compressed IPv6 and prefix 0 accepted");
    expect(networkInterface.configureIPv6("::1", 128), "IPv6 prefix 128 accepted");
    expect(!networkInterface.configureIPv6("hello:world", 64), "invalid IPv6 words rejected");
    expect(!networkInterface.configureIPv6("abc:def:zzz", 64), "invalid IPv6 hex rejected");
    expect(!networkInterface.configureIPv6("2001:db8::1", 129), "IPv6 prefix above 128 rejected");
}

void testAtomicConfiguration() {
    netlab::Device device("pc", "PC", netlab::DeviceRole::Endpoint,
                          {{"G0", 1000}}, {"DHCP_CLIENT"});
    netlab::InterfaceConfiguration initial{"G0", "192.168.1.10", "255.255.255.0",
                                           "192.168.1.1", "2001:db8::10", 64,
                                           netlab::NetworkInterface::SwitchportMode::Access, 10};
    expect(device.applyInterfaceConfiguration(initial), "initial atomic configuration applies");

    netlab::InterfaceConfiguration invalid{"G0", "10.0.0.10", "255.255.255.0",
                                           "10.0.0.1", "2001:db8::20", 64,
                                           netlab::NetworkInterface::SwitchportMode::Trunk, 5000};
    expect(!device.applyInterfaceConfiguration(invalid), "invalid VLAN rejects whole configuration");
    const auto* networkInterface = device.interfaceNamed("G0");
    expect(networkInterface->ipv4Address() == "192.168.1.10", "IPv4 unchanged after atomic failure");
    expect(device.defaultGateway() == "192.168.1.1", "gateway unchanged after atomic failure");
    expect(networkInterface->ipv6Address() == "2001:db8::10", "IPv6 unchanged after atomic failure");
    expect(networkInterface->accessVLAN() == 10, "VLAN unchanged after atomic failure");
}

void testRoutingAndCapabilities() {
    netlab::Device router("router", "Router", netlab::DeviceRole::Router,
                          {{"G0/0", 1000}, {"G0/1", 1000}},
                          {"L3_ROUTING", "NAT", "ACL", "DHCP_SERVER"});
    router.interfaceNamed("G0/0")->configureIPv4("10.1.0.1", "255.255.0.0");
    router.interfaceNamed("G0/1")->configureIPv4("10.1.2.1", "255.255.255.0");
    expect(router.addStaticRoute("0.0.0.0", "0.0.0.0", "203.0.113.1"), "default route accepted");
    auto route = router.bestRoute("10.1.2.99");
    expect(route && route->destination == "10.1.2.0", "longest prefix connected route selected");
    expect(router.routingTable().size() == 3, "connected and default routes present");
    expect(router.setNATEnabled(true, "203.0.113.10"), "NAT-capable router enables NAT");
    expect(router.addACLRule({true, "any", "10.1.2.99", "allow test"}), "valid ACL accepted");

    auto pc = endpoint("pc-cap", "PC-Cap");
    expect(!pc.setDHCPServer(true, "10.0.0.0", "255.255.255.0", "10.0.0.1", "10.0.0.53"),
           "endpoint cannot become DHCP server");
    expect(!pc.setNATEnabled(true, "203.0.113.20"), "endpoint cannot enable NAT");
    expect(!pc.addDNSRecord("host.netlab", "10.0.0.10"), "endpoint cannot add DNS records");
}

void testLinkAndVLAN() {
    auto first = endpoint("vpc1", "VPC1");
    auto second = endpoint("vpc2", "VPC2");
    netlab::Device firstSwitch("sw1", "Switch1", netlab::DeviceRole::Switch,
                               {{"P1", 1000}, {"P2", 1000}}, {"L2_SWITCHING", "VLAN"});
    netlab::Device secondSwitch("sw2", "Switch2", netlab::DeviceRole::Switch,
                                {{"P1", 1000}, {"P2", 1000}}, {"L2_SWITCHING", "VLAN"});
    first.interfaceNamed("G0")->configureIPv4("172.16.10.10", "255.255.255.0");
    second.interfaceNamed("G0")->configureIPv4("172.16.10.20", "255.255.255.0");
    firstSwitch.interfaceNamed("P1")->configureAccessVLAN(10);
    firstSwitch.interfaceNamed("P2")->configureTrunk(10, {10});
    secondSwitch.interfaceNamed("P1")->configureTrunk(10, {10});
    secondSwitch.interfaceNamed("P2")->configureAccessVLAN(10);
    auto firstLink = activeLink("l1", first, "G0", firstSwitch, "P1");
    auto trunkLink = activeLink("l2", firstSwitch, "P2", secondSwitch, "P1");
    auto lastLink = activeLink("l3", secondSwitch, "P2", second, "G0");
    netlab::SimulationEngine engine;
    for (netlab::Device* device : {&first, &firstSwitch, &secondSwitch, &second}) engine.addDevice(*device);
    engine.addLink(firstLink); engine.addLink(trunkLink); engine.addLink(lastLink);
    expect(engine.ping(first, "172.16.10.20").success, "VLAN 10 traverses matching trunk");
    auto conflictingLink = activeLink("conflict", first, "G0", second, "G0");
    expect(!engine.addLink(conflictingLink), "connected interface cannot be reused");

    trunkLink.setState(netlab::LinkState::Down);
    expect(!engine.ping(first, "172.16.10.20").success, "down link blocks forwarding");
    trunkLink.setState(netlab::LinkState::Initializing);
    expect(!engine.ping(first, "172.16.10.20").success, "initializing link blocks forwarding");
    trunkLink.setState(netlab::LinkState::Up);
    secondSwitch.interfaceNamed("P2")->configureAccessVLAN(20);
    expect(!engine.ping(first, "172.16.10.20").success, "VLAN isolation blocks mismatched access port");
}

std::string requestLease(const std::string& network, const std::string& mask,
                         const std::string& gateway) {
    auto client = endpoint("client", "Client");
    auto dhcpServer = server("server", "Server");
    dhcpServer.interfaceNamed("G0")->configureIPv4(gateway, mask);
    expect(dhcpServer.setDHCPServer(true, network, mask, gateway, gateway), "DHCP pool configuration accepted");
    auto link = activeLink("dhcp-link", client, "G0", dhcpServer, "G0");
    netlab::SimulationEngine engine;
    engine.addDevice(client); engine.addDevice(dhcpServer); engine.addLink(link);
    netlab::ServiceResult result = engine.requestDHCP(client);
    expect(result.success, "reachable DHCP request succeeds for " + mask);
    expect(!client.interfaceNamed("G0")->ipv4Address().empty(), "DHCP address applied");
    expect(client.defaultGateway() == gateway, "DHCP gateway applied");
    expect(client.dnsServer() == gateway, "DHCP DNS server applied");
    return client.interfaceNamed("G0")->ipv4Address();
}

void testDHCP() {
    expect(requestLease("192.168.50.0", "255.255.255.0", "192.168.50.1") == "192.168.50.100",
           "/24 pool starts at host 100");
    expect(requestLease("10.20.0.0", "255.255.0.0", "10.20.0.1") == "10.20.0.100",
           "/16 pool uses real subnet mask");
    expect(requestLease("192.168.60.0", "255.255.255.128", "192.168.60.1") == "192.168.60.100",
           "/25 pool excludes broadcast and uses host 100");

    auto client1 = endpoint("client1", "Client1");
    auto client2 = endpoint("client2", "Client2");
    auto dhcpServer = server("pool", "PoolServer");
    netlab::Device poolSwitch("pool-switch", "PoolSwitch", netlab::DeviceRole::Switch,
                              {{"P1", 1000}, {"P2", 1000}, {"P3", 1000}},
                              {"L2_SWITCHING", "VLAN"});
    dhcpServer.interfaceNamed("G0")->configureIPv4("192.168.70.100", "255.255.255.192");
    expect(dhcpServer.setDHCPServer(true, "192.168.70.64", "255.255.255.192",
                                    "192.168.70.65", "192.168.70.100"), "/26 pool accepted");
    auto link1 = activeLink("pool-l1", client1, "G0", poolSwitch, "P1");
    auto link2 = activeLink("pool-l2", client2, "G0", poolSwitch, "P2");
    auto serverLink = activeLink("pool-l3", dhcpServer, "G0", poolSwitch, "P3");
    netlab::SimulationEngine engine;
    engine.addDevice(client1); engine.addDevice(client2); engine.addDevice(dhcpServer); engine.addDevice(poolSwitch);
    engine.addLink(link1); engine.addLink(link2); engine.addLink(serverLink);
    expect(engine.requestDHCP(client1).success && engine.requestDHCP(client2).success,
           "multiple /26 leases succeed");
    std::set<std::string> leases;
    for (const auto& lease : dhcpServer.dhcpLeases()) leases.insert(lease.second);
    expect(leases.size() == 2, "DHCP leases are unique");
    expect(!leases.count("192.168.70.64") && !leases.count("192.168.70.127") &&
           !leases.count("192.168.70.65") && !leases.count("192.168.70.100"),
           "network, broadcast, gateway, and used server address excluded");

    auto disconnected = endpoint("disconnected", "Disconnected");
    netlab::SimulationEngine disconnectedEngine;
    disconnectedEngine.addDevice(disconnected); disconnectedEngine.addDevice(dhcpServer);
    expect(!disconnectedEngine.requestDHCP(disconnected).success,
           "disconnected DHCP server cannot lease");
}

void testDNS() {
    auto client = endpoint("dns-client", "DNSClient");
    auto dnsServer = server("dns-server", "DNSServer");
    client.interfaceNamed("G0")->configureIPv4("10.30.0.10", "255.255.255.0");
    dnsServer.interfaceNamed("G0")->configureIPv4("10.30.0.53", "255.255.255.0");
    expect(client.setDNSServer("10.30.0.53"), "client DNS server accepted");
    expect(dnsServer.addDNSRecord("server.netlab", "10.30.0.53"), "DNS record accepted");
    auto link = activeLink("dns-link", client, "G0", dnsServer, "G0");
    netlab::SimulationEngine engine;
    engine.addDevice(client); engine.addDevice(dnsServer); engine.addLink(link);
    expect(engine.ping(client, "server.netlab").success, "configured reachable DNS resolves name");
    expect(!engine.ping(client, "missing.netlab").success, "missing DNS record rejected");
    link.setState(netlab::LinkState::Down);
    expect(!engine.ping(client, "server.netlab").success, "unreachable DNS server rejected");
}

}  // namespace

int main() {
    testIPv4AndIPv6();
    testAtomicConfiguration();
    testRoutingAndCapabilities();
    testLinkAndVLAN();
    testDHCP();
    testDNS();
    expect(NLRunUISmokeTests(), "AppKit demo, DHCP, DNS Ping, deletion, and palette smoke test");
    if (failures != 0) {
        std::cerr << failures << " of " << checks << " checks failed.\n";
        return EXIT_FAILURE;
    }
    std::cout << "NetLabTests passed: " << checks << " checks.\n";
    return EXIT_SUCCESS;
}
