#pragma once

#include "prism/core/container/optional.hpp"
#include "prism/core/container/vector.hpp"
#include "prism/core/string/string.hpp"

#include <cassert>

namespace prism {

/**
 * @brief A hash table with nested scoping for symbol table lookups.
 *
 * Supports push/pop of scopes for nested name resolution. Lookups search from
 * the innermost scope outward, returning the first match found.
 *
 * @tparam T The value type stored in the table.
 */
template <typename T>
class ScopedHashTable {
    struct Entry {
        String name;  ///< The key name.
        T value;      ///< The associated value.
    };

    struct Scope {
        Vector<Entry> entries;  ///< Entries in this scope level.
    };

    Vector<Scope> scopes_;  ///< Stack of scopes (last is current).

public:
    /** @brief Construct with a single global scope. */
    ScopedHashTable() { push_scope(); }

    /** @brief Push a new empty scope onto the scope stack. */
    auto push_scope() -> void { scopes_.emplace_back(); }

    /** @brief Pop the innermost scope. Asserts if only the global scope remains. */
    auto pop_scope() -> void {
        assert(!scopes_.empty() && "Cannot pop global scope");
        scopes_.pop_back();
    }

    /**
     * @brief Insert a name-value pair into the current scope.
     *
     * @param name The key name.
     * @param value The value to associate with the name.
     * @return true if insertion succeeded; false if the name already exists in the current scope.
     */
    auto insert(const String& name, const T& value) -> bool {
        auto& current = scopes_.back().entries;
        for (size_t i = 0; i < current.size(); ++i) {
            if (current[i].name == name) return false;
        }
        current.push_back(Entry{name, value});
        return true;
    }

    /**
     * @brief Look up a name, searching from the innermost scope outward.
     *
     * @param name The key name to look up.
     * @return Pointer to the value if found, or nullptr if not found in any scope.
     */
    auto lookup(const String& name) -> T* {
        for (size_t scope = scopes_.size(); scope > 0; --scope) {
            auto& entries = scopes_[scope - 1].entries;
            for (size_t i = entries.size(); i > 0; --i) {
                if (entries[i - 1].name == name) return &entries[i - 1].value;
            }
        }
        return nullptr;
    }

    /**
     * @brief Check whether a name exists in any scope.
     *
     * @param name The key name to check.
     * @return true if the name is found in any scope.
     */
    auto contains(const String& name) const -> bool {
        for (size_t scope = scopes_.size(); scope > 0; --scope) {
            const auto& entries = scopes_[scope - 1].entries;
            for (size_t i = 0; i < entries.size(); ++i) {
                if (entries[i].name == name) return true;
            }
        }
        return false;
    }

    /**
     * @brief Check whether a name exists only in the current (innermost) scope.
     *
     * @param name The key name to check.
     * @return true if the name is present in the current scope.
     */
    auto in_current_scope(const String& name) const -> bool {
        const auto& entries = scopes_.back().entries;
        for (size_t i = 0; i < entries.size(); ++i) {
            if (entries[i].name == name) return true;
        }
        return false;
    }

    /** @brief Get the current number of nested scopes. */
    auto scope_depth() const -> size_t { return scopes_.size(); }
};

}  // namespace prism
