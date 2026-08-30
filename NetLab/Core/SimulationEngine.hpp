#pragma once

#include "Device.hpp"

#include <string>
#include <vector>

namespace netlab {

struct EthernetConnection {
    Device* firstDevice;
    std::string firstInterface;
    Device* secondDevice;
    std::string secondInterface;
};

struct SimulationEvent {
    std::string stage;
    std::string detail;
};

struct PingResult {
    bool success = false;
    std::string summary;
    std::vector<SimulationEvent> events;
};

struct ServiceResult {
    bool success = false;
    std::string summary;
    std::vector<SimulationEvent> events;
};

class SimulationEngine final {
public:
    void addDevice(Device& device);
    void addEthernetConnection(EthernetConnection connection);
    PingResult ping(Device& source, const std::string& destinationAddress);
    ServiceResult requestDHCP(Device& client);
    std::string advancedStatus(const Device& device) const;

private:
    struct PathHop {
        Device* device;
        std::string incomingInterface;
        std::string outgoingInterface;
    };

    Device* findDeviceWithAddress(const std::string& address) const noexcept;
    Device* findDefaultGateway(const Device& source) const noexcept;
    std::string resolveDestination(const std::string& addressOrName, std::vector<SimulationEvent>& events) const;
    std::vector<PathHop> findLayer2Path(Device& source, Device& destination) const;
    bool validateVLANPath(const std::vector<PathHop>& path, int& vlanID,
                          std::vector<SimulationEvent>& events) const;
    bool processLayer2Segment(Device& sender, Device& receiver, const std::string& arpAddress,
                              std::vector<SimulationEvent>& events);

    std::vector<Device*> devices_;
    std::vector<EthernetConnection> connections_;
};

}  // namespace netlab
