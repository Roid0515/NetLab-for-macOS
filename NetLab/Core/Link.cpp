#include "Link.hpp"

#include <utility>

namespace netlab {

Link::Link(std::string identifier, LinkEndpoint first, LinkEndpoint second, int speedMbps)
    : identifier_(std::move(identifier)),
      first_(std::move(first)),
      second_(std::move(second)),
      speedMbps_(speedMbps) {}

}  // namespace netlab
