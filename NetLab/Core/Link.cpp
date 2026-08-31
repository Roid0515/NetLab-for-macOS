#include "Link.hpp"

#include <utility>

namespace netlab {

Link::Link(std::string identifier, LinkEndpoint first, LinkEndpoint second, int speedMbps)
    : Link(std::move(identifier), std::move(first), std::move(second),
           LinkType::Ethernet, speedMbps) {}

Link::Link(std::string identifier, LinkEndpoint first, LinkEndpoint second,
           LinkType type, int speedMbps)
    : identifier_(std::move(identifier)),
      first_(std::move(first)),
      second_(std::move(second)),
      type_(type),
      speedMbps_(speedMbps) {}

}  // namespace netlab
