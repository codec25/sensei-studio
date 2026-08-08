#include "sensei/core/Transport.hpp"

// Transport is header-implemented for inlining/atomics.
// This translation unit exists so the library always has a compiled object.
namespace sensei::core {
namespace {
[[maybe_unused]] constexpr int kTransportLibraryAnchor = 1;
}
} // namespace sensei::core
