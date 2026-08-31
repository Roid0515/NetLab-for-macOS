#pragma once

#include <string>
#include <vector>

namespace netlab {

class NetworkInterface final {
public:
    enum class SwitchportMode { Access, Trunk };
    NetworkInterface(std::string name, std::string macAddress, int speedMbps);

    const std::string& name() const noexcept { return name_; }
    const std::string& macAddress() const noexcept { return macAddress_; }
    int speedMbps() const noexcept { return speedMbps_; }
    bool adminUp() const noexcept { return adminUp_; }
    void setAdminUp(bool enabled) noexcept { adminUp_ = enabled; }
    const std::string& ipv4Address() const noexcept { return ipv4Address_; }
    const std::string& subnetMask() const noexcept { return subnetMask_; }
    bool hasIPv4Configuration() const noexcept;
    bool configureIPv4(const std::string& address, const std::string& subnetMask);
    static bool isValidIPv4Configuration(const std::string& address,
                                         const std::string& subnetMask) noexcept;
    const std::string& ipv6Address() const noexcept { return ipv6Address_; }
    int ipv6PrefixLength() const noexcept { return ipv6PrefixLength_; }
    bool configureIPv6(const std::string& address, int prefixLength);
    static bool isValidIPv6Configuration(const std::string& address,
                                         int prefixLength) noexcept;
    void clearIPv6Configuration() noexcept;

    SwitchportMode switchportMode() const noexcept { return switchportMode_; }
    int accessVLAN() const noexcept { return accessVLAN_; }
    int nativeVLAN() const noexcept { return nativeVLAN_; }
    const std::vector<int>& allowedVLANs() const noexcept { return allowedVLANs_; }
    bool configureAccessVLAN(int vlanID);
    bool configureTrunk(int nativeVLAN, std::vector<int> allowedVLANs);
    bool allowsVLAN(int vlanID) const noexcept;

private:
    std::string name_;
    std::string macAddress_;
    int speedMbps_;
    bool adminUp_ = true;
    std::string ipv4Address_;
    std::string subnetMask_;
    std::string ipv6Address_;
    int ipv6PrefixLength_ = 0;
    SwitchportMode switchportMode_ = SwitchportMode::Access;
    int accessVLAN_ = 1;
    int nativeVLAN_ = 1;
    std::vector<int> allowedVLANs_ = {1};
};

}  // namespace netlab
