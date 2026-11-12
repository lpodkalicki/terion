/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_FORM_STATE_H
#define TERION_FORM_STATE_H

#include <charconv>
#include <cmath>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <terion/form_value.h>
#include <terion/state.h>

namespace terion {

namespace detail {

using FormValidator = std::function<std::optional<std::string>(const FormValue&)>;

struct FormFieldDefinition
{
    std::string name;
    FormValue initial_value = std::string{};
    bool required = false;
    FormValidator validator = {};
}; /* End of struct FormFieldDefinition */

struct FormSnapshot
{
    FormValues values;
    std::unordered_map<std::string,std::string> errors;
    std::unordered_set<std::string> touched;
    std::unordered_map<std::string,std::string> number_drafts;
    bool submitted = false;
}; /* End of struct FormSnapshot */

inline std::string formatNumber(double value)
{
    if (!std::isfinite(value)) throw std::invalid_argument("Nonfinite number in form");
    std::string buffer(64,'\0');
    const auto result = std::to_chars(buffer.data(),buffer.data() + buffer.size(),value);
    if (result.ec != std::errc{}) throw std::invalid_argument("Cannot format number");
    buffer.resize(static_cast<std::size_t>(result.ptr - buffer.data()));
    return buffer;
}

inline std::optional<double> parseNumber(const std::string& value)
{
    if (value.empty()) return std::nullopt;
    double result = 0;
    const auto parsed = std::from_chars(value.data(),value.data() + value.size(),result);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() || !std::isfinite(result))
        return std::nullopt;
    return result;
}

inline std::string toText(const FormValue& value)
{
    if (const auto* text = std::get_if<std::string>(&value)) return *text;
    if (const auto* integer = std::get_if<std::int64_t>(&value)) return std::to_string(*integer);
    if (const auto* number = std::get_if<double>(&value)) return formatNumber(*number);
    if (const auto* boolean = std::get_if<bool>(&value)) return *boolean ? "true" : "false";
    return {};
}

class FormState
{
public:
    explicit FormState(std::vector<FormFieldDefinition> fields) :
        definitions_(std::move(fields)),snapshot_(initialSnapshot(definitions_))
    {
        for (const auto& field : definitions_)
            if (field.name.empty() || !field_names_.insert(field.name).second)
                throw std::invalid_argument("Form requires unique nonempty field names");
    }

    [[nodiscard]] FormSnapshot snapshot() const { return snapshot_.get(); }
    [[nodiscard]] FormValues values() const { return snapshot_.get().values; }
    [[nodiscard]] FormValue value(const std::string& name) const
    {
        (void)requireField(name);
        return snapshot_.get().values.at(name);
    }
    [[nodiscard]] std::string error(const std::string& name) const
    {
        (void)requireField(name);
        const auto current = snapshot_.get();
        const auto it = current.errors.find(name);
        return it == current.errors.end() ? std::string{} : it->second;
    }

    [[nodiscard]] std::string displayValue(const std::string& name) const
    {
        const auto current = snapshot_.get();
        const auto it = current.number_drafts.find(name);
        if (it != current.number_drafts.end()) return it->second;
        return toText(current.values.at(name));
    }

    void setNumberText(const std::string& name,std::string value)
    {
        const auto& field = requireField(name);
        auto current = snapshot_.get();
        const auto parsed = parseNumber(value);
        current.values[name] = parsed ? FormValue(*parsed) : FormValue(std::monostate{});
        current.number_drafts[name] = std::move(value);
        current.touched.insert(name);
        setError(current,field);
        snapshot_.set(std::move(current));
    }

    void setValue(const std::string& name,FormValue value)
    {
        const auto& field = requireField(name);
        auto current = snapshot_.get();
        current.values[name] = std::move(value);
        current.number_drafts.erase(name);
        current.touched.insert(name);
        setError(current,field);
        snapshot_.set(std::move(current));
    }

    [[nodiscard]] bool validate()
    {
        auto current = snapshot_.get();
        current.submitted = true;
        current.errors.clear();
        for (const auto& field : definitions_) setError(current,field);
        const bool valid = current.errors.empty();
        snapshot_.set(std::move(current));
        return valid;
    }

    [[nodiscard]] bool submit(const std::function<void(const FormValues&)>& on_submit)
    {
        if (!validate()) return false;
        if (on_submit) on_submit(values());
        return true;
    }

    void reset() { snapshot_.set(initialSnapshot(definitions_)); }

private:
    [[nodiscard]] static FormSnapshot initialSnapshot(const std::vector<FormFieldDefinition>& fields)
    {
        FormSnapshot result;
        for (const auto& field : fields) result.values[field.name] = field.initial_value;
        return result;
    }

    [[nodiscard]] const FormFieldDefinition& requireField(const std::string& name) const
    {
        for (const auto& field : definitions_)
            if (field.name == name) return field;
        throw std::out_of_range("Unknown form field: " + name);
    }

    static void setError(FormSnapshot& snapshot,const FormFieldDefinition& field)
    {
        const auto& value = snapshot.values.at(field.name);
        const auto* str = std::get_if<std::string>(&value);
        std::optional<std::string> error;
        if (field.required && (std::holds_alternative<std::monostate>(value) ||
            (str && str->find_first_not_of(" \t\r\n") == std::string::npos) ||
            (std::holds_alternative<bool>(value) && !std::get<bool>(value))))
            error = "This field is required";
        else if (field.validator) error = field.validator(value);
        if (error && !error->empty()) snapshot.errors[field.name] = std::move(*error);
        else snapshot.errors.erase(field.name);
    }

    std::vector<FormFieldDefinition> definitions_;
    std::unordered_set<std::string> field_names_;
    State<FormSnapshot> snapshot_;
}; /* End of class FormState */

} /* End of namespace detail */

} /* End of namespace terion */

#endif /* TERION_FORM_STATE_H */
