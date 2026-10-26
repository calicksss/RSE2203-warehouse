#include "core/Config.hpp"

namespace wh {

std::string toString(RoutingMethod m)
{
    switch (m) {
        case RoutingMethod::AsGiven:      return "As given";
        case RoutingMethod::Greedy:       return "Greedy NN";
        case RoutingMethod::Backtracking: return "Backtracking";
    }
    return "?";
}

std::string toString(AssignmentMethod m)
{
    switch (m) {
        case AssignmentMethod::FirstAvailable: return "First available";
        case AssignmentMethod::Nearest:        return "Nearest";
    }
    return "?";
}

} // namespace wh
