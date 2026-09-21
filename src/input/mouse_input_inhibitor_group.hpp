#pragma once

#include <memory>
#include <vector>

#include "mouse_capture_controller.hpp"

namespace vkShade
{
    class MouseInputInhibitorGroup final : public MouseInputInhibitor
    {
    public:
        void add(std::unique_ptr<MouseInputInhibitor> inhibitor);

        bool inhibit() override;
        void reconcile() override;
        void restore() override;
        bool is_ready_for_capture() const override;
        bool permits_pointer_constraint() const override;

    private:
        std::vector<std::unique_ptr<MouseInputInhibitor>> m_inhibitors;
        std::vector<MouseInputInhibitor*> m_activeInhibitors;
    };
} // namespace vkShade
