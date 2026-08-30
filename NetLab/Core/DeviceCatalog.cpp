#include "DeviceCatalog.hpp"

namespace netlab {

const std::vector<DeviceDefinition>& DeviceCatalog::defaultDefinitions() {
    static const std::vector<DeviceDefinition> definitions = {
        {"generic-router", DeviceCategory::Router, "Generic Router", "Router",
         {{"G0/0", InterfaceType::GigabitEthernet, 1000},
          {"G0/1", InterfaceType::GigabitEthernet, 1000}},
         {"L3_ROUTING", "DHCP_SERVER", "NAT", "ACL"}},
        {"l2-switch", DeviceCategory::Switch, "L2 Switch", "Switch",
         {{"G0/1", InterfaceType::GigabitEthernet, 1000},
          {"G0/2", InterfaceType::GigabitEthernet, 1000},
          {"F0/1", InterfaceType::FastEthernet, 100},
          {"F0/2", InterfaceType::FastEthernet, 100}},
         {"L2_SWITCHING", "VLAN", "STP"}},
        {"firewall", DeviceCategory::Security, "Learning Firewall", "Firewall",
         {{"INSIDE", InterfaceType::GigabitEthernet, 1000},
          {"OUTSIDE", InterfaceType::GigabitEthernet, 1000}},
         {"L3_ROUTING", "FIREWALL", "NAT", "ACL", "VPN"}},
        {"wireless-ap", DeviceCategory::Wireless, "Wireless AP", "AP",
         {{"G0", InterfaceType::GigabitEthernet, 1000},
          {"WLAN0", InterfaceType::Ethernet, 300}},
         {"WIRELESS", "VLAN"}},
        {"generic-server", DeviceCategory::Server, "Services Server", "Server",
         {{"G0", InterfaceType::GigabitEthernet, 1000}},
         {"DHCP_SERVER", "DNS_SERVER"}},
        {"desktop-pc", DeviceCategory::Endpoint, "Desktop PC", "PC",
         {{"G0", InterfaceType::GigabitEthernet, 1000}},
         {"DHCP_CLIENT"}},
    };
    return definitions;
}

}  // namespace netlab
