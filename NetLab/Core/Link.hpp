#pragma once

#include <string>

namespace netlab {

enum class LinkType { Ethernet, Fiber, Serial, Wireless, LogicalTunnel };
enum class LinkState { Initializing, Down, Up };

struct LinkEndpoint {
    std::string deviceIdentifier;
    std::string interfaceName;
};

// Pure C++ link model shared by the UI today and the simulation engine later.
class Link final {
public:
    Link(std::string identifier, LinkEndpoint first, LinkEndpoint second, int speedMbps);
    Link(std::string identifier, LinkEndpoint first, LinkEndpoint second,
         LinkType type, int speedMbps);

    const std::string& identifier() const noexcept { return identifier_; }
    const LinkEndpoint& firstEndpoint() const noexcept { return first_; }
    const LinkEndpoint& secondEndpoint() const noexcept { return second_; }
    LinkType type() const noexcept { return type_; }
    int speedMbps() const noexcept { return speedMbps_; }
    LinkState state() const noexcept { return state_; }
    void setState(LinkState state) noexcept { state_ = state; }

private:
    std::string identifier_;
    LinkEndpoint first_;
    LinkEndpoint second_;
    LinkType type_;
    int speedMbps_;
    LinkState state_ = LinkState::Initializing;
};

}  // namespace netlab
