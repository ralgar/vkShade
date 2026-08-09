#pragma once

#include "panel.hpp"

#include <string>
#include <vector>

namespace vkShade
{
    class LogPanel : public GuiPanel
    {
    public:
        void render() override;

    private:
        std::vector<std::string> m_messages;
        double m_nextRefresh = 0.0;
        bool m_paused = false;
        bool m_follow = true;
    };
} // namespace vkShade
