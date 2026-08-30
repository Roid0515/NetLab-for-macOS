#import <AppKit/AppKit.h>

#import "AppDelegate.h"
#import "MainWindowController.h"
#include "../Core/DeviceCatalog.hpp"
#include "../Core/Link.hpp"
#include "../Core/SimulationEngine.hpp"

// NSApplication does not retain its delegate. Keep it alive for the full process lifetime.
static AppDelegate *NLApplicationDelegate;

static void NLInstallMainMenu(void) {
    NSMenu *menuBar = [[NSMenu alloc] init];
    NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
    [menuBar addItem:appMenuItem];
    NSApp.mainMenu = menuBar;

    NSMenu *appMenu = [[NSMenu alloc] initWithTitle:@"NetLab"];
    NSString *quitTitle = [NSString stringWithFormat:@"Quit %@", NSProcessInfo.processInfo.processName];
    NSMenuItem *quitItem = [[NSMenuItem alloc] initWithTitle:quitTitle
                                                    action:@selector(terminate:)
                                             keyEquivalent:@"q"];
    [appMenu addItem:quitItem];
    appMenuItem.submenu = appMenu;
}

int main(int argc, const char *argv[]) {
    @autoreleasepool {
        if (argc == 2 && strcmp(argv[1], "--self-test") == 0) {
            const auto& definitions = netlab::DeviceCatalog::defaultDefinitions();
            if (definitions.size() != 6) return 2;
            for (const auto& definition : definitions) {
                if (definition.identifier.empty() || definition.displayName.empty() ||
                    definition.defaultNamePrefix.empty() || definition.interfaces.empty()) return 3;
            }
            netlab::Link testLink("test-link",
                                  {"device-a", "G0/0"},
                                  {"device-b", "F0/1"}, 100);
            if (testLink.speedMbps() != 100 || testLink.state() != netlab::LinkState::Initializing) return 5;
            testLink.setState(netlab::LinkState::Up);
            if (testLink.state() != netlab::LinkState::Up) return 6;
            netlab::Device pc1("pc1", "PC1", netlab::DeviceRole::Endpoint, {{"G0", 1000}});
            netlab::Device switch1("switch1", "Switch1", netlab::DeviceRole::Switch,
                                   {{"G0/1", 1000}, {"G0/2", 1000}});
            netlab::Device pc2("pc2", "PC2", netlab::DeviceRole::Endpoint, {{"G0", 1000}});
            if (!pc1.interfaceNamed("G0")->configureIPv4("192.168.1.10", "255.255.255.0") ||
                !pc2.interfaceNamed("G0")->configureIPv4("192.168.1.20", "255.255.255.0")) return 7;
            netlab::SimulationEngine engine;
            engine.addDevice(pc1);
            engine.addDevice(switch1);
            engine.addDevice(pc2);
            engine.addEthernetConnection({&pc1, "G0", &switch1, "G0/1"});
            engine.addEthernetConnection({&switch1, "G0/2", &pc2, "G0"});
            netlab::PingResult pingResult = engine.ping(pc1, "192.168.1.20");
            if (!pingResult.success || pc1.arpTable().empty() || switch1.macAddressTable().size() != 2) return 8;

            // Milestone 4: connected routing, default gateway, static/default route, NAT and ACL.
            netlab::Device routedPC1("r-pc1", "RoutedPC1", netlab::DeviceRole::Endpoint, {{"G0", 1000}});
            netlab::Device router("router", "Router1", netlab::DeviceRole::Router,
                                  {{"G0/0", 1000}, {"G0/1", 1000}});
            netlab::Device routedPC2("r-pc2", "RoutedPC2", netlab::DeviceRole::Endpoint, {{"G0", 1000}});
            routedPC1.interfaceNamed("G0")->configureIPv4("10.0.1.10", "255.255.255.0");
            routedPC1.setDefaultGateway("10.0.1.1");
            router.interfaceNamed("G0/0")->configureIPv4("10.0.1.1", "255.255.255.0");
            router.interfaceNamed("G0/1")->configureIPv4("10.0.2.1", "255.255.255.0");
            router.addStaticRoute("0.0.0.0", "0.0.0.0", "203.0.113.1");
            router.setNATEnabled(true, "203.0.113.10");
            router.addACLRule({true, "any", "any", "permit lab"});
            routedPC2.interfaceNamed("G0")->configureIPv4("10.0.2.20", "255.255.255.0");
            routedPC2.setDefaultGateway("10.0.2.1");
            routedPC2.addDNSRecord("host.netlab", "10.0.2.20");
            netlab::SimulationEngine routedEngine;
            routedEngine.addDevice(routedPC1); routedEngine.addDevice(router); routedEngine.addDevice(routedPC2);
            routedEngine.addEthernetConnection({&routedPC1, "G0", &router, "G0/0"});
            routedEngine.addEthernetConnection({&router, "G0/1", &routedPC2, "G0"});
            netlab::PingResult routedPing = routedEngine.ping(routedPC1, "host.netlab");
            if (!routedPing.success || router.routingTable().size() != 3 || router.natTranslations().empty()) {
                NSLog(@"M4 failed: %s routes=%lu nat=%lu", routedPing.summary.c_str(),
                      router.routingTable().size(), router.natTranslations().size());
                for (const auto& event : routedPing.events) NSLog(@"[%s] %s", event.stage.c_str(), event.detail.c_str());
                return 9;
            }

            // Milestone 5: access/trunk forwarding and VLAN isolation.
            netlab::Device vlanPC1("vpc1", "VLANPC1", netlab::DeviceRole::Endpoint, {{"G0", 1000}});
            netlab::Device vlanSW1("vsw1", "VSwitch1", netlab::DeviceRole::Switch, {{"P1", 1000}, {"P2", 1000}});
            netlab::Device vlanSW2("vsw2", "VSwitch2", netlab::DeviceRole::Switch, {{"P1", 1000}, {"P2", 1000}});
            netlab::Device vlanPC2("vpc2", "VLANPC2", netlab::DeviceRole::Endpoint, {{"G0", 1000}});
            vlanPC1.interfaceNamed("G0")->configureIPv4("172.16.10.10", "255.255.255.0");
            vlanPC2.interfaceNamed("G0")->configureIPv4("172.16.10.20", "255.255.255.0");
            vlanSW1.interfaceNamed("P1")->configureAccessVLAN(10);
            vlanSW1.interfaceNamed("P2")->configureTrunk(10, {10, 20});
            vlanSW2.interfaceNamed("P1")->configureTrunk(10, {10, 20});
            vlanSW2.interfaceNamed("P2")->configureAccessVLAN(10);
            netlab::SimulationEngine vlanEngine;
            vlanEngine.addDevice(vlanPC1); vlanEngine.addDevice(vlanSW1); vlanEngine.addDevice(vlanSW2); vlanEngine.addDevice(vlanPC2);
            vlanEngine.addEthernetConnection({&vlanPC1, "G0", &vlanSW1, "P1"});
            vlanEngine.addEthernetConnection({&vlanSW1, "P2", &vlanSW2, "P1"});
            vlanEngine.addEthernetConnection({&vlanSW2, "P2", &vlanPC2, "G0"});
            if (!vlanEngine.ping(vlanPC1, "172.16.10.20").success) return 10;
            vlanSW2.interfaceNamed("P2")->configureAccessVLAN(20);
            if (vlanEngine.ping(vlanPC1, "172.16.10.20").success) return 11;

            // Milestone 6 and 7: services and representative advanced protocol state.
            netlab::Device dhcpClient("dhcp-client", "DHCPClient", netlab::DeviceRole::Endpoint, {{"G0", 1000}});
            netlab::Device serviceServer("services", "Services", netlab::DeviceRole::Server, {{"G0", 1000}});
            serviceServer.setDHCPServer(true, "192.168.50.0", "255.255.255.0", "192.168.50.1", "192.168.50.53");
            netlab::SimulationEngine servicesEngine;
            servicesEngine.addDevice(dhcpClient); servicesEngine.addDevice(serviceServer);
            if (!servicesEngine.requestDHCP(dhcpClient).success || serviceServer.dhcpLeases().size() != 1) return 12;
            router.enableSTP(true);
            router.setDynamicRoutingProtocol("OSPF/RIP");
            router.interfaceNamed("G0/0")->configureIPv6("2001:db8::1", 64);
            router.setWireless("NetLab-Class", true);
            router.setFirewallEnabled(true);
            router.setVPNTunnel("198.51.100.2", true);
            std::string advanced = routedEngine.advancedStatus(router);
            if (advanced.find("OSPF/RIP converged") == std::string::npos ||
                advanced.find("Tunnel up") == std::string::npos || advanced.find("2001:db8::1") == std::string::npos) return 13;
            NSLog(@"NetLab self-test passed: %lu device definitions", definitions.size());
            return 0;
        }
        if (argc == 2 && strcmp(argv[1], "--ui-self-test") == 0) {
            NSApplication *testApplication = [NSApplication sharedApplication];
            testApplication.activationPolicy = NSApplicationActivationPolicyProhibited;
            MainWindowController *controller = [[MainWindowController alloc] init];
            BOOL passed = [controller runLayoutSelfTest];
            NSLog(@"NetLab UI self-test %@", passed ? @"passed" : @"failed");
            return passed ? 0 : 4;
        }

        NSApplication *application = [NSApplication sharedApplication];
        application.activationPolicy = NSApplicationActivationPolicyRegular;
        NLInstallMainMenu();
        NLApplicationDelegate = [[AppDelegate alloc] init];
        application.delegate = NLApplicationDelegate;
        [application run];
    }
    return 0;
}
