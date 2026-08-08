#pragma once

#include "sensei/core/commands/Command.hpp"

#include <memory>
#include <vector>

namespace sensei::core {

class CommandHistory
{
public:
    static constexpr std::size_t kDefaultCapacity = 100;

    explicit CommandHistory(std::size_t capacity = kDefaultCapacity)
        : capacity_(capacity > 0 ? capacity : kDefaultCapacity)
    {
    }

    bool execute(Project& project, std::unique_ptr<Command> command)
    {
        if (command == nullptr)
            return false;

        if (! command->perform(project))
            return false;

        undoStack_.push_back(std::move(command));
        if (undoStack_.size() > capacity_)
            undoStack_.erase(undoStack_.begin());

        redoStack_.clear();
        return true;
    }

    bool undo(Project& project)
    {
        if (undoStack_.empty())
            return false;

        auto command = std::move(undoStack_.back());
        undoStack_.pop_back();
        command->undo(project);
        redoStack_.push_back(std::move(command));
        return true;
    }

    bool redo(Project& project)
    {
        if (redoStack_.empty())
            return false;

        auto command = std::move(redoStack_.back());
        redoStack_.pop_back();
        if (! command->perform(project))
        {
            // Keep command on redo if replay failed (should be rare).
            redoStack_.push_back(std::move(command));
            return false;
        }

        undoStack_.push_back(std::move(command));
        return true;
    }

    [[nodiscard]] bool canUndo() const noexcept { return ! undoStack_.empty(); }
    [[nodiscard]] bool canRedo() const noexcept { return ! redoStack_.empty(); }
    [[nodiscard]] std::size_t undoSize() const noexcept { return undoStack_.size(); }
    [[nodiscard]] std::size_t redoSize() const noexcept { return redoStack_.size(); }

private:
    std::size_t capacity_;
    std::vector<std::unique_ptr<Command>> undoStack_;
    std::vector<std::unique_ptr<Command>> redoStack_;
};

} // namespace sensei::core
