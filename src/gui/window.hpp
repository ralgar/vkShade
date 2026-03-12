#pragma once

namespace vkShade
{
    // Abstract base class for GUI windows
    class GuiWindow
    {
    public:
        virtual ~GuiWindow() = default;

        bool visible() const { return m_visible; }
        void visible(bool value) { m_visible = value; }

        virtual void render() = 0;

    protected:
        GuiWindow() = default;

        bool m_visible = false;
    };
} // namespace vkShade
