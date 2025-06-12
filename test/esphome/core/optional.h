#pragma once

#include <experimental/optional>

// Fakes the esphome optional type with std experimental/optional

namespace esphome
{
    template <typename T>
    using optional = std::experimental::optional<T>;
    /*        using opt_null_t = std::experimental::nullopt_t;
    constexpr auto nullopt = std::experimental::nullopt;*/
}