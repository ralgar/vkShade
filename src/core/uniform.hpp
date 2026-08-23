#pragma once

#include <cstdint>
#include <string>

#include <glm/glm.hpp>

namespace vkShade
{
    struct Uniform
    {
        // The type of the uniform as understood by vkShade.
        //
        // This intentionally does NOT expose reshadefx::type. ReShade
        // reflection is converted to this enum at the boundary.
        enum class Type
        {
            Float, Float2, Float3, Float4,
            Int,   Int2,   Int3,   Int4,
            Uint,  Uint2,  Uint3,  Uint4,
            Bool,  Bool2,  Bool3,  Bool4,
        };

        std::string name;
        uint32_t size {0};
        uint32_t offset {0};
        Type type;

        // Runtime -> compile-time
        //
        // Uniform::Type is known only at runtime, but some operations
        // (preset.get<T>(), on_uniform_changed<T>(), etc.) need an actual
        // C++ type as a template argument.
        //
        // This function is the single place where we bridge that gap.
        //
        // Example:
        //
        //     Uniform::dispatch_type(uniform.type, [&]<typename T>(std::type_identity<T>)
        //     {
        //         preset.get<T>(...);
        //     });
        //
        // Inside the lambda, T is the concrete C++ type corresponding
        // to the runtime Uniform::Type.
        template <typename Func>
        static void dispatch_type(Type type, Func&& func)
        {
            switch (type)
            {
                case Type::Float:       return func(std::type_identity<float>{});
                case Type::Float2:      return func(std::type_identity<glm::vec2>{});
                case Type::Float3:      return func(std::type_identity<glm::vec3>{});
                case Type::Float4:      return func(std::type_identity<glm::vec4>{});

                case Type::Int:         return func(std::type_identity<int32_t>{});
                case Type::Int2:        return func(std::type_identity<glm::ivec2>{});
                case Type::Int3:        return func(std::type_identity<glm::ivec3>{});
                case Type::Int4:        return func(std::type_identity<glm::ivec4>{});

                case Type::Uint:        return func(std::type_identity<uint32_t>{});
                case Type::Uint2:       return func(std::type_identity<glm::uvec2>{});
                case Type::Uint3:       return func(std::type_identity<glm::uvec3>{});
                case Type::Uint4:       return func(std::type_identity<glm::uvec4>{});

                case Type::Bool:        return func(std::type_identity<bool>{});
                case Type::Bool2:       return func(std::type_identity<glm::bvec2>{});
                case Type::Bool3:       return func(std::type_identity<glm::bvec3>{});
                case Type::Bool4:       return func(std::type_identity<glm::bvec4>{});
            }
        }
    };

    // Compile-time C++ type -> runtime Uniform::Type
    //
    // This is the inverse of dispatch_type().
    //
    // For example:
    //
    //     uniform_type_v<float>      == Uniform::Type::Float
    //     uniform_type_v<glm::vec3>  == Uniform::Type::Float3
    //
    // The primary template intentionally has no definition. If somebody
    // tries to use an unsupported C++ type, that is a compile-time error.
    template <typename T>
    struct UniformType;

    template <> struct UniformType<float>       : std::integral_constant<Uniform::Type, Uniform::Type::Float> {};
    template <> struct UniformType<glm::vec2>   : std::integral_constant<Uniform::Type, Uniform::Type::Float2> {};
    template <> struct UniformType<glm::vec3>   : std::integral_constant<Uniform::Type, Uniform::Type::Float3> {};
    template <> struct UniformType<glm::vec4>   : std::integral_constant<Uniform::Type, Uniform::Type::Float4> {};

    template <> struct UniformType<int32_t>     : std::integral_constant<Uniform::Type, Uniform::Type::Int> {};
    template <> struct UniformType<glm::ivec2>  : std::integral_constant<Uniform::Type, Uniform::Type::Int2> {};
    template <> struct UniformType<glm::ivec3>  : std::integral_constant<Uniform::Type, Uniform::Type::Int3> {};
    template <> struct UniformType<glm::ivec4>  : std::integral_constant<Uniform::Type, Uniform::Type::Int4> {};

    template <> struct UniformType<uint32_t>    : std::integral_constant<Uniform::Type, Uniform::Type::Uint> {};
    template <> struct UniformType<glm::uvec2>  : std::integral_constant<Uniform::Type, Uniform::Type::Uint2> {};
    template <> struct UniformType<glm::uvec3>  : std::integral_constant<Uniform::Type, Uniform::Type::Uint3> {};
    template <> struct UniformType<glm::uvec4>  : std::integral_constant<Uniform::Type, Uniform::Type::Uint4> {};

    template <> struct UniformType<bool>        : std::integral_constant<Uniform::Type, Uniform::Type::Bool> {};
    template <> struct UniformType<glm::bvec2>  : std::integral_constant<Uniform::Type, Uniform::Type::Bool2> {};
    template <> struct UniformType<glm::bvec3>  : std::integral_constant<Uniform::Type, Uniform::Type::Bool3> {};
    template <> struct UniformType<glm::bvec4>  : std::integral_constant<Uniform::Type, Uniform::Type::Bool4> {};

    // Convenient variable-template form, analogous to std::is_same_v, etc.
    template <typename T>
    inline constexpr Uniform::Type uniform_type_v = UniformType<T>::value;
} // namespace vkShade
