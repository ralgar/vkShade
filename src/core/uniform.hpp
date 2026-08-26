#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>

#include <glm/glm.hpp>
#include <magic_enum/magic_enum.hpp>

namespace vkShade
{
    struct Uniform
    {
        // The type of the uniform as understood by vkShade.
        //
        // This intentionally does NOT expose reshadefx::type. ReShade
        // reflection is converted to this enum at the boundary.
        enum class BaseType
        {
            Float,
            Int,
            Uint,
            Bool,
        };

        enum class UiType
        {
            Input,
            Drag,
            Slider,
            Combo,
            Radio,
            Color,
            Button,
        };

        using Scalar  = std::variant<float, int32_t, uint32_t, bool>;

        std::string name;
        uint32_t size {0};
        uint32_t offset {0};
        BaseType baseType;
        uint32_t components {1};  // 1, 2, 3, or 4

        std::optional<UiType> uiType;
        std::optional<Scalar> uiMin;
        std::optional<Scalar> uiMax;
        std::optional<Scalar> uiStep;
        std::vector<std::string> uiItems;
        std::string uiLabel;
        std::string uiTooltip;
        std::string uiCategory;
        bool        uiCategoryClosed {false};
        std::string uiUnits;
        bool hidden   {false};
        bool disabled {false};
        bool noReset  {false};

        // Stores an optional default for each base type component
        std::array<std::optional<Scalar>, 4> defaultValues;

        // Runtime -> compile-time
        //
        // Uniform::BaseType is known only at runtime, but some operations
        // (preset.get<T>(), on_uniform_changed<T>(), etc.) need an actual
        // C++ type as a template argument.
        //
        // This function is the single place where we bridge that gap.
        //
        // Example:
        //
        //     Uniform::dispatch_type(uniform.baseType, 4, [&]<typename T>(std::type_identity<T>)
        //     {
        //         preset.get<T>(...);
        //     });
        //
        // Inside the lambda, T is the concrete C++ type corresponding
        // to the runtime Uniform::Type.
        template <typename Func>
        static void dispatch_type(BaseType baseType, uint32_t components, Func&& func)
        {
            switch (baseType)
            {
                case BaseType::Float:
                    switch (components)
                    {
                        case 1: return func(std::type_identity<float>{});
                        case 2: return func(std::type_identity<glm::vec2>{});
                        case 3: return func(std::type_identity<glm::vec3>{});
                        case 4: return func(std::type_identity<glm::vec4>{});
                    }
                    break;

                case BaseType::Int:
                    switch (components)
                    {
                        case 1: return func(std::type_identity<int32_t>{});
                        case 2: return func(std::type_identity<glm::ivec2>{});
                        case 3: return func(std::type_identity<glm::ivec3>{});
                        case 4: return func(std::type_identity<glm::ivec4>{});
                    }
                    break;

                case BaseType::Uint:
                    switch (components)
                    {
                        case 1: return func(std::type_identity<uint32_t>{});
                        case 2: return func(std::type_identity<glm::uvec2>{});
                        case 3: return func(std::type_identity<glm::uvec3>{});
                        case 4: return func(std::type_identity<glm::uvec4>{});
                    }
                    break;

                case BaseType::Bool:
                    switch (components)
                    {
                        case 1: return func(std::type_identity<bool>{});
                        case 2: return func(std::type_identity<glm::bvec2>{});
                        case 3: return func(std::type_identity<glm::bvec3>{});
                        case 4: return func(std::type_identity<glm::bvec4>{});
                    }
                    break;
            }
        }
    };

    // Compile-time C++ type -> runtime Uniform::Type
    //
    // This is the inverse of dispatch_type().
    //
    // For example:
    //
    //     UniformTraits<glm::vec3>::Scalar     == float
    //     UniformTraits<glm::vec3>::base       == Uniform::Type::Float
    //     UniformTraits<glm::vec3>::components == 3
    //
    // The primary template intentionally has no definition. If somebody
    // tries to use an unsupported C++ type, that is a compile-time error.
    template <typename T>
    struct UniformTraits;

    template <>
    struct UniformTraits<float>
    {
        using Scalar = float;
        static constexpr Uniform::BaseType base = Uniform::BaseType::Float;
        static constexpr uint32_t components = 1;
    };

    template <>
    struct UniformTraits<glm::vec2>
    {
        using Scalar = float;
        static constexpr Uniform::BaseType base = Uniform::BaseType::Float;
        static constexpr uint32_t components = 2;
    };

    template <>
    struct UniformTraits<glm::vec3>
    {
        using Scalar = float;
        static constexpr Uniform::BaseType base = Uniform::BaseType::Float;
        static constexpr uint32_t components = 3;
    };

    template <>
    struct UniformTraits<glm::vec4>
    {
        using Scalar = float;
        static constexpr Uniform::BaseType base = Uniform::BaseType::Float;
        static constexpr uint32_t components = 4;
    };

    template <>
    struct UniformTraits<int32_t>
    {
        using Scalar = int32_t;
        static constexpr Uniform::BaseType base = Uniform::BaseType::Int;
        static constexpr uint32_t components = 1;
    };

    template <>
    struct UniformTraits<glm::ivec2>
    {
        using Scalar = int32_t;
        static constexpr Uniform::BaseType base = Uniform::BaseType::Int;
        static constexpr uint32_t components = 2;
    };

    template <>
    struct UniformTraits<glm::ivec3>
    {
        using Scalar = int32_t;
        static constexpr Uniform::BaseType base = Uniform::BaseType::Int;
        static constexpr uint32_t components = 3;
    };

    template <>
    struct UniformTraits<glm::ivec4>
    {
        using Scalar = int32_t;
        static constexpr Uniform::BaseType base = Uniform::BaseType::Int;
        static constexpr uint32_t components = 4;
    };

    template <>
    struct UniformTraits<uint32_t>
    {
        using Scalar = uint32_t;
        static constexpr Uniform::BaseType base = Uniform::BaseType::Uint;
        static constexpr uint32_t components = 1;
    };

    template <>
    struct UniformTraits<glm::uvec2>
    {
        using Scalar = uint32_t;
        static constexpr Uniform::BaseType base = Uniform::BaseType::Uint;
        static constexpr uint32_t components = 2;
    };

    template <>
    struct UniformTraits<glm::uvec3>
    {
        using Scalar = uint32_t;
        static constexpr Uniform::BaseType base = Uniform::BaseType::Uint;
        static constexpr uint32_t components = 3;
    };

    template <>
    struct UniformTraits<glm::uvec4>
    {
        using Scalar = uint32_t;
        static constexpr Uniform::BaseType base = Uniform::BaseType::Uint;
        static constexpr uint32_t components = 4;
    };

    template <>
    struct UniformTraits<bool>
    {
        using Scalar = bool;
        static constexpr Uniform::BaseType base = Uniform::BaseType::Bool;
        static constexpr uint32_t components = 1;
    };

    template <>
    struct UniformTraits<glm::bvec2>
    {
        using Scalar = bool;
        static constexpr Uniform::BaseType base = Uniform::BaseType::Bool;
        static constexpr uint32_t components = 2;
    };

    template <>
    struct UniformTraits<glm::bvec3>
    {
        using Scalar = bool;
        static constexpr Uniform::BaseType base = Uniform::BaseType::Bool;
        static constexpr uint32_t components = 3;
    };

    template <>
    struct UniformTraits<glm::bvec4>
    {
        using Scalar = bool;
        static constexpr Uniform::BaseType base = Uniform::BaseType::Bool;
        static constexpr uint32_t components = 4;
    };
} // namespace vkShade
