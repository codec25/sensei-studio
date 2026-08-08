#pragma once

#include "sensei/core/commands/Command.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace sensei::core {

class CompoundCommand final : public Command
{
public:
    explicit CompoundCommand(std::string name)
        : name_(std::move(name))
    {
    }

    void add(std::unique_ptr<Command> command)
    {
        if (command != nullptr)
            children_.push_back(std::move(command));
    }

    [[nodiscard]] std::string name() const override { return name_; }

    bool perform(Project& project) override
    {
        performedCount_ = 0;
        for (auto& child : children_)
        {
            if (! child->perform(project))
            {
                // Rollback successful prefix.
                for (std::size_t i = performedCount_; i > 0; --i)
                    children_[i - 1]->undo(project);
                performedCount_ = 0;
                return false;
            }
            ++performedCount_;
        }
        return true;
    }

    void undo(Project& project) override
    {
        for (std::size_t i = performedCount_; i > 0; --i)
            children_[i - 1]->undo(project);
    }

private:
    std::string name_;
    std::vector<std::unique_ptr<Command>> children_;
    std::size_t performedCount_ = 0;
};

} // namespace sensei::core
