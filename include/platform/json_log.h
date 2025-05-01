/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#include <fmt/core.h>
#include <gsl/gsl-lite.hpp>
#include <nlohmann/json.hpp>
#include <platform/cb_allocator.h>
#include <platform/ordered_map.h>

namespace cb::logger {
namespace detail {

/**
 * The allocator used for JSON log object. They are always allocated from the
 * global arena. This makes it easier to reason about the memory usage of the
 * JSON log messages while avoiding the need for defensive copying.
 */
template <typename T>
struct Allocator : cb::NoArenaAllocator<T> {};

/**
 * Like std::string, but using our allocator. Note that constructing from
 * std::string always requires a copy (even from std::string&&).
 */
using String = std::
        basic_string<char, std::char_traits<char>, detail::Allocator<char>>;

/**
 * Map type used for objects stored in a LogJson object.
 */
template <typename Key, typename T, typename KeyCompare, typename Allocator>
using Map = cb::OrderedMap<
        Key,
        T,
        KeyCompare,
        std::vector<std::pair<Key, T>,
                    typename std::allocator_traits<Allocator>::
                            template rebind_alloc<std::pair<Key, T>>>>;

} // namespace detail

template <typename T, typename SFINAE = void>
struct JsonSerializer : nlohmann::adl_serializer<T, SFINAE> {};

/**
 * The nlohmann::basic_json<> template used by cb::logger::Json.
 * Can be used in the signature of:
 *   void to_json(BasicJsonType j, T value);
 * To customize how objects are represented in log messages.
 */
using BasicJsonType = nlohmann::basic_json<
        detail::Map,
        std::vector,
        detail::String,
        bool,
        std::int64_t,
        std::uint64_t,
        double,
        detail::Allocator,
        JsonSerializer,
        std::vector<std::uint8_t, detail::Allocator<std::uint8_t>>>;

/**
 * Custom JSON type used for logging with the following properties:
 * - uses cb::OrderedMap to preserve ordering of JSON keys
 * - uses cb::NoArenaAllocator for all internal allocations
 *
 * Inheriting from nlohmann::basic_json<> allows us to:
 * - forward-declare class Json
 * - have a non-allocating destructor
 * - simplify the usage of initializer lists in macros, by preferring moving the
 *   someValue in `Json{someValue}` versus creating a 1-element array.
 */
class Json : public BasicJsonType {
public:
    /**
     * Create Json from an initializer_list.
     */
    Json(initializer_list_t init)
        // Avoid creating 1-element object arrays.
        // Makes Json b{std::move(a)} move a into b, as opposed to creating [a].
        : BasicJsonType(init.size() == 1 && (*init.begin())->is_object()
                                ? BasicJsonType(*init.begin())
                                : BasicJsonType(init)) {
    }

    Json(const Json& other)
        : BasicJsonType(static_cast<const BasicJsonType&>(other)) {
    }

    Json(Json&& other) : BasicJsonType(static_cast<BasicJsonType&&>(other)) {
    }

    /**
     * Normally, we could use `using BasicJsonType::BasicJsonType` to inherit
     * the base class constructors. However, MSVC seems to then generate
     * Json::Json(const Json&) which calls BasicJsonType(const Json&) instead of
     * BasicJsonType(const BasicJsonType&).
     *
     * It gets mixed up with the templated constructor of BasicJsonType, which
     * then ends up in a complicated chain of template magic which results in
     * nlohmann::basic_json<> throwing a type_error.
     *
     * Instead of inheriting constructors, use an explicit constructor taking
     * BasicJsonType + some of the constructors available in basic_json<>.
     */

    Json(const value_t v = value_t::null) : BasicJsonType(v) {
    }

    Json(BasicJsonType other) : BasicJsonType(std::move(other)) {
    }

    Json& operator=(const Json&) = default;
    Json& operator=(Json&&) = default;

    ~Json() {
        destroyRecursive(*this);
    }

private:
    /**
     * Destroy any sub-objects manually. Otherwise, nlohmann::basic_json will do
     * the same using a std::stack which always allocates using std::allocator.
     */
    static void destroyRecursive(BasicJsonType& json) {
        const auto type = json.type();
        if (type == value_t::array) {
            auto& array = json.template get_ref<array_t&>();
            for (auto& sub : json.template get_ref<array_t&>()) {
                destroyRecursive(sub);
            }
            array.clear();
        } else if (type == value_t::object) {
            auto& object = json.template get_ref<object_t&>();
            for (auto& sub : json.template get_ref<object_t&>()) {
                destroyRecursive(sub.second);
            }
            object.clear();
        }
    }
};

/**
 * Due to how C++ templates work, sometimes, nlohmann::json might decide to
 * "convert" from logger::Json to logger::BasicJsonType the specialised
 * basic_json<> and base of logger::Json. To avoid it getting into incorrect or
 * inefficient conversion logic, we can explicitly define the efficient
 * conversion here.
 */
template <>
struct JsonSerializer<Json> {
    template <typename BasicJsonType>
    static void to_json(BasicJsonType& j, const Json& val) {
        j = static_cast<const logger::BasicJsonType&>(val);
    }

    template <typename BasicJsonType>
    static void to_json(BasicJsonType& j, Json&& val) {
        j = static_cast<logger::BasicJsonType&&>(val);
    }
};

} // namespace cb::logger

template <>
struct fmt::formatter<cb::logger::Json> : formatter<string_view> {
    auto format(cb::logger::Json json, format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", json.dump());
    }
};

// Automatic conversions do not need to be included separately.
#include <platform/json_log_conversions.h>
