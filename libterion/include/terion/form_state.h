/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_FORM_STATE_H
#define TERION_FORM_STATE_H

#include <charconv>
#include <cmath>
#include <cstddef>
#include <functional>
#include <memory>
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

enum class FormValidationMode { OnChange, OnBlur, OnSubmit };

struct FormStatus
{
    bool dirty = false;
    bool valid = true;
    bool submitted = false;
    bool submitting = false;
    std::size_t submit_count = 0;
    std::size_t touched_count = 0;
    std::size_t error_count = 0;
}; /* End of struct FormStatus */

namespace detail {

using FormValidator = std::function<std::optional<std::string>(const FormValue&)>;
using FormValuesValidator =
    std::function<std::optional<std::string>(const FormValue&,const FormValues&)>;

struct FormFieldDefinition
{
    std::string name;
    FormValue initial_value = std::string{};
    bool required = false;
    FormValidator validator = {};
    FormValuesValidator validator_with_values = {};
}; /* End of struct FormFieldDefinition */

struct FormSnapshot
{
    FormValues values;
    std::unordered_map<std::string,std::string> errors;
    std::unordered_set<std::string> touched;
    std::unordered_set<std::string> changed;
    std::unordered_set<std::string> dirty;
    std::unordered_map<std::string,std::string> number_drafts;
    bool submitted = false;
    bool submitting = false;
    std::size_t submit_count = 0;
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
    explicit FormState(std::vector<FormFieldDefinition> fields,
        FormValidationMode mode = FormValidationMode::OnChange) :
        definitions_(std::move(fields)),mode_(mode),snapshot_(initialSnapshot(definitions_))
    {
        for (const auto& field : definitions_)
            if (field.name.empty() || !field_names_.insert(field.name).second)
                throw std::invalid_argument("Form requires unique nonempty field names");
        auto current = snapshot_.get();
        refreshErrors(current);
        snapshot_.set(std::move(current));
    }

    void setValidationMode(FormValidationMode mode) noexcept { mode_ = mode; }
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
    [[nodiscard]] std::string visibleError(const std::string& name) const
    {
        (void)requireField(name);
        const auto current = snapshot_.get();
        if (!current.submitted &&
            (mode_ == FormValidationMode::OnSubmit ||
                (mode_ == FormValidationMode::OnBlur && !current.touched.contains(name)) ||
                (mode_ == FormValidationMode::OnChange && !current.changed.contains(name))))
            return {};
        const auto it = current.errors.find(name);
        return it == current.errors.end() ? std::string{} : it->second;
    }
    [[nodiscard]] bool touched(const std::string& name) const
    {
        (void)requireField(name);
        return snapshot_.get().touched.contains(name);
    }
    [[nodiscard]] bool dirty(const std::string& name) const
    {
        (void)requireField(name);
        return snapshot_.get().dirty.contains(name);
    }
    [[nodiscard]] FormStatus status() const
    {
        const auto current = snapshot_.get();
        return {.dirty = !current.dirty.empty(),.valid = current.errors.empty(),
            .submitted = current.submitted,.submitting = current.submitting,
            .submit_count = current.submit_count,.touched_count = current.touched.size(),
            .error_count = current.errors.size()};
    }
    [[nodiscard]] std::unordered_map<std::string,std::string> errors() const
    {
        return snapshot_.get().errors;
    }
    [[nodiscard]] std::string firstError() const
    {
        const auto current = snapshot_.get();
        for (const auto& field : definitions_)
            if (current.errors.contains(field.name)) return field.name;
        return {};
    }
    [[nodiscard]] std::string displayValue(const std::string& name) const
    {
        (void)requireField(name);
        const auto current = snapshot_.get();
        const auto it = current.number_drafts.find(name);
        if (it != current.number_drafts.end()) return it->second;
        return toText(current.values.at(name));
    }
    void setNumberText(const std::string& name,std::string value)
    {
        (void)requireField(name);
        auto current = snapshot_.get();
        const auto parsed = parseNumber(value);
        current.values[name] = parsed ? FormValue(*parsed) : FormValue(std::monostate{});
        current.number_drafts[name] = std::move(value);
        current.changed.insert(name);
        updateDirty(current,name);
        refreshErrors(current);
        snapshot_.set(std::move(current));
    }
    void setValue(const std::string& name,FormValue value)
    {
        (void)requireField(name);
        auto current = snapshot_.get();
        current.values[name] = std::move(value);
        current.number_drafts.erase(name);
        current.changed.insert(name);
        updateDirty(current,name);
        refreshErrors(current);
        snapshot_.set(std::move(current));
    }
    void markTouched(const std::string& name)
    {
        (void)requireField(name);
        auto current = snapshot_.get();
        if (current.touched.insert(name).second) snapshot_.set(std::move(current));
    }
    [[nodiscard]] bool validateField(const std::string& name)
    {
        const auto& field = requireField(name);
        auto current = snapshot_.get();
        setError(current,field);
        current.touched.insert(name);
        const bool valid = !current.errors.contains(name);
        snapshot_.set(std::move(current));
        return valid;
    }
    [[nodiscard]] bool validate()
    {
        auto current = snapshot_.get();
        current.submitted = true;
        ++current.submit_count;
        refreshErrors(current);
        const bool valid = current.errors.empty();
        snapshot_.set(std::move(current));
        return valid;
    }
    [[nodiscard]] bool submit(const std::function<void(const FormValues&)>& on_submit)
    {
        if (snapshot_.get().submitting) return false;
        if (!validate()) return false;
        if (!on_submit) return true;
        auto current = snapshot_.get();
        if (current.submitting) return false;
        current.submitting = true;
        snapshot_.set(std::move(current));
        try { on_submit(values()); }
        catch (...)
        {
            current = snapshot_.get();
            current.submitting = false;
            snapshot_.set(std::move(current));
            throw;
        }
        current = snapshot_.get();
        current.submitting = false;
        snapshot_.set(std::move(current));
        return true;
    }
    void reset()
    {
        auto current = initialSnapshot(definitions_);
        refreshErrors(current);
        snapshot_.set(std::move(current));
    }

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
    void updateDirty(FormSnapshot& current,const std::string& name) const
    {
        const auto& initial = initial_.at(name);
        const auto draft = current.number_drafts.find(name);
        const bool is_dirty = current.values.at(name) != initial ||
            (draft != current.number_drafts.end() && draft->second != toText(initial));
        if (is_dirty) current.dirty.insert(name);
        else current.dirty.erase(name);
    }
    void refreshErrors(FormSnapshot& current) const
    {
        current.errors.clear();
        for (const auto& field : definitions_) setError(current,field);
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
        if ((!error || error->empty()) && field.validator_with_values)
            error = field.validator_with_values(value,snapshot.values);
        if (error && !error->empty()) snapshot.errors[field.name] = std::move(*error);
        else snapshot.errors.erase(field.name);
    }

    std::vector<FormFieldDefinition> definitions_;
    std::unordered_set<std::string> field_names_;
    FormValidationMode mode_;
    State<FormSnapshot> snapshot_;
    FormValues initial_ = initialSnapshot(definitions_).values;
}; /* End of class FormState */

} /* End of namespace detail */

// A lightweight handle for actions outside Form's component tree. Calling a method
// before the Form has mounted (or after it unmounts) reports a logic error.
class FormController
{
public:
    [[nodiscard]] bool ready() const noexcept { return !state_.expired(); }
    [[nodiscard]] FormStatus status() const { return locked()->status(); }
    [[nodiscard]] FormValues values() const { return locked()->values(); }
    [[nodiscard]] FormValue value(const std::string& name) const { return locked()->value(name); }
    [[nodiscard]] std::string error(const std::string& name) const { return locked()->error(name); }
    [[nodiscard]] std::string firstError() const { return locked()->firstError(); }
    [[nodiscard]] std::unordered_map<std::string,std::string> errors() const
    {
        return locked()->errors();
    }
    [[nodiscard]] bool dirty(const std::string& name) const { return locked()->dirty(name); }
    [[nodiscard]] bool touched(const std::string& name) const { return locked()->touched(name); }
    void setValue(const std::string& name,FormValue value) const
    {
        auto state = locked();
        state->setValue(name,std::move(value));
        notify(*state);
    }
    void markTouched(const std::string& name) const
    {
        auto state = locked();
        state->markTouched(name);
        notify(*state);
    }
    [[nodiscard]] bool validateField(const std::string& name) const
    {
        auto state = locked();
        const bool valid = state->validateField(name);
        notify(*state);
        return valid;
    }
    [[nodiscard]] bool validate() const
    {
        auto state = locked();
        const bool valid = state->validate();
        notify(*state);
        return valid;
    }
    [[nodiscard]] bool submit(const std::function<void(const FormValues&)>& on_submit) const
    {
        auto state = locked();
        try
        {
            const bool valid = state->submit(on_submit);
            notify(*state);
            return valid;
        }
        catch (...) { notify(*state); throw; }
    }
    void reset() const
    {
        auto state = locked();
        state->reset();
        notify(*state);
    }

private:
    friend class Form;
    void attach(const std::shared_ptr<detail::FormState>& state,
        std::function<void(FormStatus)> on_change)
    {
        state_ = state;
        on_change_ = std::move(on_change);
    }
    void notify(const detail::FormState& state) const
    {
        if (on_change_) on_change_(state.status());
    }
    [[nodiscard]] std::shared_ptr<detail::FormState> locked() const
    {
        auto state = state_.lock();
        if (!state) throw std::logic_error("FormController requires a mounted Form");
        return state;
    }
    std::weak_ptr<detail::FormState> state_;
    std::function<void(FormStatus)> on_change_;
}; /* End of class FormController */

} /* End of namespace terion */

#endif /* TERION_FORM_STATE_H */
