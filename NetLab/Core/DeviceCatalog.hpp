#pragma once

#include "DeviceDefinition.hpp"

#include <vector>

namespace netlab {

// Central catalog keeps the device palette data-driven and easy to extend.
class DeviceCatalog final {
public:
    static const std::vector<DeviceDefinition>& defaultDefinitions();
};

}  // namespace netlab
