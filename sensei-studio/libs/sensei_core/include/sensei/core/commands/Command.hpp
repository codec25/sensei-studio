#pragma once

#include "sensei/core/Project.hpp"

#include <string>

namespace sensei::core {

class Command
{
public:
    virtual ~Command() = default;
    [[nodiscard]] virtual std::string name() const = 0;
    virtual bool perform(Project& project) = 0;
    virtual void undo(Project& project) = 0;
};

} // namespace sensei::core
