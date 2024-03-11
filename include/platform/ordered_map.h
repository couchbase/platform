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

#include <functional>
#include <utility>
#include <vector>

namespace cb {

/**
 * Minimal std::map-like type that can be used as the ObjectType parameter in
 * nlohmann::basic_json<>. Preserves ordering and allows key-value pairs to be
 * inserted at any position (nlohmann::ordered_map does not).
 */
template <typename Key,
          typename T,
          typename IgnoredLess = std::less<Key>,
          typename Container = std::vector<std::pair<Key, T>>>
class OrderedMap : public Container {
private:
    /**
     * SFINAE utility for OtherKey which can be compared to the actual key.
     * We want to allow const char* to be used with operator[] without
     * allocating std::string.
     */
    template <typename OtherKey>
    using ComparableToKey = decltype(
            std::equal_to<>()(std::declval<Key>(), std::declval<OtherKey>()));

public:
    using key_type = Key;
    using key_compare = std::equal_to<>;
    using mapped_type = T;

    using container_type = Container;
    using typename Container::allocator_type;
    using typename Container::const_iterator;
    using typename Container::const_reference;
    using typename Container::iterator;
    using typename Container::reference;
    using typename Container::size_type;
    using typename Container::value_type;

    OrderedMap() = default;

    template <typename It>
    OrderedMap(It first, It last) : Container{first, last} {
    }

    OrderedMap(std::initializer_list<value_type> init) : Container{init} {
    }

    using Container::emplace;
    template <typename KeyType, typename = ComparableToKey<KeyType>>
    std::pair<iterator, bool> emplace(KeyType&& key, T&& t) {
        auto it = find(static_cast<const std::decay_t<KeyType>&>(key));
        if (it != this->end()) {
            return {it, false};
        }
        Container::emplace_back(std::forward<KeyType>(key), std::forward<T>(t));
        return {std::prev(this->end()), true};
    }

    template <typename KeyType, typename = ComparableToKey<KeyType>>
    iterator find(KeyType&& key) {
        return std::find_if(this->begin(), this->end(), [&key](auto& pair) {
            return key_compare()(pair.first, key);
        });
    }

    template <typename KeyType, typename = ComparableToKey<KeyType>>
    const_iterator find(KeyType&& key) const {
        return std::find_if(this->cbegin(), this->cend(), [&key](auto& pair) {
            return key_compare()(pair.first, key);
        });
    }
};

} // namespace cb
