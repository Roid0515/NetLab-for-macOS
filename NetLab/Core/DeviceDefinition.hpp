#pragma once

#include <string>
#include <vector>

namespace netlab {

enum class DeviceRole { Endpoint, Switch, Router, Server, Firewall, WirelessAP };

enum class DeviceCategory {
    Router,
    Switch,
    Security,
    Wireless,
    Server,
    Endpoint,
};

enum class InterfaceType {
    Ethernet,
    FastEthernet,
    GigabitEthernet,
};

struct InterfaceDefinition {
    std::string name;
    InterfaceType type;
    int speedMbps;
};

// Data-only definition used by the UI and, later, by the simulation engine.
struct DeviceDefinition {
    std::string identifier;
    DeviceCategory category;
    DeviceRole role;
    std::string displayName;
    std::string defaultNamePrefix;
    std::vector<InterfaceDefinition> interfaces;
    std::vector<std::string> capabilities;
};

}  // namespace netlab
