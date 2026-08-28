#include "mouse_input_inhibitor_group.hpp"

#include <utility>

namespace vkShade
{
    void MouseInputInhibitorGroup::add(std::unique_ptr<MouseInputInhibitor> inhibitor)
    {
        m_inhibitors.push_back(std::move(inhibitor));
    }

    bool MouseInputInhibitorGroup::inhibit()
    {
        if (!m_activeInhibitors.empty())
            return true;

        for (const auto& inhibitor : m_inhibitors)
        {
            if (inhibitor->inhibit())
                m_activeInhibitors.push_back(inhibitor.get());
        }

        return !m_activeInhibitors.empty();
    }

    void MouseInputInhibitorGroup::restore()
    {
        // Unwind in reverse activation order so overlapping adapters restore
        // the application state they observed rather than an intermediate one.
        for (auto it = m_activeInhibitors.rbegin(); it != m_activeInhibitors.rend(); ++it)
            (*it)->restore();

        m_activeInhibitors.clear();
    }
} // namespace vkShade
