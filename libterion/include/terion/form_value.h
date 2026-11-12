/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_FORM_VALUE_H
#define TERION_FORM_VALUE_H

#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

namespace terion {

using FormValue = std::variant<std::monostate,std::string,std::int64_t,double,bool>;

class FormValues : public std::unordered_map<std::string,FormValue>
{
public:
    using std::unordered_map<std::string,FormValue>::unordered_map;

    template<typename T>
    [[nodiscard]] T get(std::string_view name) const
    {
        const auto& value = at(std::string(name));
        if constexpr (std::is_integral_v<T> && !std::is_same_v<T,bool>)
        {
            long double number = 0;
            if (const auto* integer = std::get_if<std::int64_t>(&value))
                number = static_cast<long double>(*integer);
            else if (const auto* real = std::get_if<double>(&value))
                number = static_cast<long double>(*real);
            else throw std::bad_variant_access();
            if (!std::isfinite(number) || std::trunc(number) != number ||
                number < static_cast<long double>(std::numeric_limits<T>::lowest()) ||
                number > static_cast<long double>(std::numeric_limits<T>::max()))
                throw std::out_of_range("Form value cannot be represented as requested integer type");
            return static_cast<T>(number);
        }
        else if constexpr (std::is_same_v<T,double>)
        {
            if (const auto* integer = std::get_if<std::int64_t>(&value))
                return static_cast<double>(*integer);
            return std::get<double>(value);
        }
        else return std::get<T>(value);
    }
}; /* End of class FormValues */

} /* End of namespace terion */

#endif /* TERION_FORM_VALUE_H */
