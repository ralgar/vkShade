#pragma once

namespace vkShade
{
    // Abstract base class for GUI panels
    class GuiPanel
    {
    public:
        virtual ~GuiPanel() = default;

        virtual void render() = 0;

    protected:
        GuiPanel() = default;
    };
} // namespace vkShade
