#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>

namespace vkShade
{
    struct StatisticsSnapshot
    {
        double current {0.0};
        double average {0.0};
        double minimum {0.0};
        double maximum {0.0};
    };

    template<std::size_t WindowSize>
    class RollingStatistics
    {
        static_assert(WindowSize > 0);

    public:
        void add(double sample)
        {
            if (m_count == WindowSize)
                m_sum -= m_samples[m_next];
            else
                ++m_count;

            m_samples[m_next] = sample;
            m_sum += sample;
            m_current = sample;
            m_next = (m_next + 1) % WindowSize;
        }

        void reset()
        {
            m_count = 0;
            m_next = 0;
            m_sum = 0.0;
            m_current = 0.0;
        }

        [[nodiscard]]
        std::optional<StatisticsSnapshot> snapshot() const
        {
            if (m_count == 0)
                return std::nullopt;

            const auto end = m_samples.begin() + static_cast<std::ptrdiff_t>(m_count);
            const auto [minimum, maximum] = std::minmax_element(m_samples.begin(), end);
            return StatisticsSnapshot {
                .current = m_current,
                .average = m_sum / static_cast<double>(m_count),
                .minimum = *minimum,
                .maximum = *maximum,
            };
        }

    private:
        std::array<double, WindowSize> m_samples {};
        std::size_t m_count {0};
        std::size_t m_next {0};
        double m_sum {0.0};
        double m_current {0.0};
    };
} // namespace vkShade
