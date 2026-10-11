#pragma once

#include "kodak/core/Finding.h"
#include "kodak/core/Geometry.h"
#include "kodak/core/Image.h"

#include <stop_token>
#include <vector>

namespace kodak::core {

/// A source of redaction proposals: sensitive text now, faces later.
class Detector {
public:
    virtual ~Detector() = default;

    /// Proposals inside `region`, in image coordinates, all enabled. Returns
    /// early, possibly with fewer proposals, once `stop` is requested.
    virtual std::vector<Finding> detect(const Image& image, const Rect& region, std::stop_token stop) = 0;
};

} // namespace kodak::core
