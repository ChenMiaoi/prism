#pragma once

#include "prism/core/utility/hash.hpp"
#include "prism/core/utility/move.hpp"

#include <cstddef>
#include <cstdint>

namespace prism {

/** @brief An open-addressing hash map using Swiss-table-style control bytes. */
template <typename K, typename V, typename Hash = DefaultHash>
class HashMap {
    static constexpr size_t GroupSize = 16;   ///< Number of slots probed per group.
    static constexpr uint8_t Empty = 0x80;    ///< Control byte indicating an empty slot.
    static constexpr uint8_t Deleted = 0xFE;  ///< Control byte indicating a deleted (tombstone) slot.

    /** @brief Internal slot storing the control byte, hash fragment, key, and value. */
    struct Slot {
        uint8_t control;  ///< Control byte for fast probing.
        uint8_t h2;       ///< Secondary hash fragment for disambiguation.
        K key;            ///< The stored key.
        V value;          ///< The stored value.

        /** @brief Constructs an empty slot. */
        Slot() : control(Empty), h2(0) {}
        /** @brief Default destructor. */
        ~Slot() {}
    };

public:
    /** @brief Iterator type (currently a pointer to the HashMap itself). */
    using iterator = HashMap*;

    /** @brief Default-constructs an empty hash map. */
    HashMap() : slots_(nullptr), size_(0), capacity_(0), growth_threshold_(0) {}

    /** @brief Destructor. Destroys all occupied slots and frees memory. */
    ~HashMap() {
        if (slots_) {
            for (size_t i = 0; i < capacity_; ++i) {
                if (slots_[i].control != Empty && slots_[i].control != Deleted) {
                    slots_[i].key.~K();
                    slots_[i].value.~V();
                }
            }
            ::operator delete(slots_);
        }
    }

    /** @brief Copy construction is deleted. */
    HashMap(const HashMap&) = delete;
    /** @brief Copy assignment is deleted. */
    HashMap& operator=(const HashMap&) = delete;

    /** @brief Move constructor. Transfers ownership of slots.
     *  @param other The hash map to move from. */
    HashMap(HashMap&& other) noexcept
        : slots_(other.slots_)
        , size_(other.size_)
        , capacity_(other.capacity_)
        , growth_threshold_(other.growth_threshold_) {
        other.slots_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
        other.growth_threshold_ = 0;
    }

    /** @brief Move assignment. Transfers ownership of slots.
     *  @param other The hash map to move from.
     *  @return Reference to this hash map. */
    HashMap& operator=(HashMap&& other) noexcept {
        if (this != &other) {
            if (slots_) {
                for (size_t i = 0; i < capacity_; ++i) {
                    if (slots_[i].control != Empty && slots_[i].control != Deleted) {
                        slots_[i].key.~K();
                        slots_[i].value.~V();
                    }
                }
                ::operator delete(slots_);
            }
            slots_ = other.slots_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            growth_threshold_ = other.growth_threshold_;
            other.slots_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
            other.growth_threshold_ = 0;
        }
        return *this;
    }

    /** @brief Inserts or retrieves the value for @p key.
     *  @param key The key to look up or insert.
     *  @return Reference to the value associated with @p key. */
    V& operator[](const K& key) {
        auto result = find_or_insert(key);
        return result->value;
    }

    /** @brief Inserts or retrieves the value for a moved @p key.
     *  @param key The key to look up or insert.
     *  @return Reference to the value associated with @p key. */
    V& operator[](K&& key) {
        auto result = find_or_insert(move(key));
        return result->value;
    }

    /** @brief Finds the slot containing @p key.
     *  @param key The key to search for.
     *  @return Pointer to the slot, or nullptr if not found. */
    Slot* find(const K& key) {
        if (capacity_ == 0) return nullptr;
        uint8_t h2 = hash_h2(key);
        size_t index = hash_index(key);

        for (size_t probe = 0; probe < capacity_; probe += GroupSize) {
            size_t base = (index + probe) % capacity_;
            for (size_t i = 0; i < GroupSize; ++i) {
                size_t slot_idx = (base + i) % capacity_;
                if (slots_[slot_idx].control == Empty) return nullptr;
                if (slots_[slot_idx].control == h2 && slots_[slot_idx].key == key) {
                    return &slots_[slot_idx];
                }
            }
        }
        return nullptr;
    }

    /** @brief Inserts a key-value pair, or updates the value if the key exists.
     *  @param key The key to insert.
     *  @param value The value to associate with @p key.
     *  @return Pointer to the resulting slot. */
    Slot* insert(const K& key, const V& value) {
        if (size_ >= growth_threshold_) {
            rehash(capacity_ == 0 ? GroupSize * 2 : capacity_ * 2);
        }
        uint8_t h2 = hash_h2(key);
        size_t index = hash_index(key);

        for (size_t probe = 0; probe < capacity_; probe += GroupSize) {
            size_t base = (index + probe) % capacity_;
            for (size_t i = 0; i < GroupSize; ++i) {
                size_t slot_idx = (base + i) % capacity_;
                if (slots_[slot_idx].control == Empty || slots_[slot_idx].control == Deleted) {
                    new (&slots_[slot_idx].key) K(key);
                    new (&slots_[slot_idx].value) V(value);
                    slots_[slot_idx].h2 = h2;
                    slots_[slot_idx].control = h2;
                    ++size_;
                    return &slots_[slot_idx];
                }
                if (slots_[slot_idx].control == h2 && slots_[slot_idx].key == key) {
                    slots_[slot_idx].value = value;
                    return &slots_[slot_idx];
                }
            }
        }
        return nullptr;
    }

    /** @brief Removes the element with the given key.
     *  @param key The key to remove.
     *  @return True if the element was found and removed, false otherwise. */
    bool erase(const K& key) {
        Slot* slot = find(key);
        if (!slot) return false;
        slot->key.~K();
        slot->value.~V();
        slot->control = Deleted;
        slot->h2 = 0;
        --size_;
        return true;
    }

    /** @brief Checks whether the map contains the given key.
     *  @param key The key to search for.
     *  @return True if @p key is present. */
    bool contains(const K& key) const { return const_cast<HashMap*>(this)->find(key) != nullptr; }

    /** @brief Returns the number of elements in the map. */
    size_t size() const noexcept { return size_; }
    /** @brief Returns true if the map is empty. */
    bool empty() const noexcept { return size_ == 0; }

    /** @brief Removes all elements from the map. */
    void clear() {
        for (size_t i = 0; i < capacity_; ++i) {
            if (slots_[i].control != Empty && slots_[i].control != Deleted) {
                slots_[i].key.~K();
                slots_[i].value.~V();
                slots_[i].control = Empty;
            }
        }
        size_ = 0;
    }

private:
    /** @brief Finds an existing slot for @p key or inserts a new one.
     *  @param key The key to find or insert.
     *  @return Pointer to the resulting slot. */
    Slot* find_or_insert(const K& key) {
        if (size_ >= growth_threshold_) {
            rehash(capacity_ == 0 ? GroupSize * 2 : capacity_ * 2);
        }
        uint8_t h2 = hash_h2(key);
        size_t index = hash_index(key);
        Slot* first_deleted = nullptr;

        for (size_t probe = 0; probe < capacity_; probe += GroupSize) {
            size_t base = (index + probe) % capacity_;
            for (size_t i = 0; i < GroupSize; ++i) {
                size_t slot_idx = (base + i) % capacity_;
                if (slots_[slot_idx].control == Empty) {
                    if (first_deleted) {
                        new (&first_deleted->key) K(key);
                        new (&first_deleted->value) V();
                        first_deleted->h2 = h2;
                        first_deleted->control = h2;
                        ++size_;
                        return first_deleted;
                    }
                    new (&slots_[slot_idx].key) K(key);
                    new (&slots_[slot_idx].value) V();
                    slots_[slot_idx].h2 = h2;
                    slots_[slot_idx].control = h2;
                    ++size_;
                    return &slots_[slot_idx];
                }
                if (slots_[slot_idx].control == Deleted && !first_deleted) {
                    first_deleted = &slots_[slot_idx];
                }
                if (slots_[slot_idx].control == h2 && slots_[slot_idx].key == key) {
                    return &slots_[slot_idx];
                }
            }
        }
        if (first_deleted) {
            new (&first_deleted->key) K(key);
            new (&first_deleted->value) V();
            first_deleted->h2 = h2;
            first_deleted->control = h2;
            ++size_;
            return first_deleted;
        }
        return nullptr;
    }

    /** @brief Rehashes the map into a new buffer of @p new_capacity slots.
     *  @param new_capacity The desired new capacity. */
    void rehash(size_t new_capacity) {
        Slot* old_slots = slots_;
        size_t old_capacity = capacity_;

        capacity_ = new_capacity;
        growth_threshold_ = static_cast<size_t>(capacity_ * 14 / 16);
        slots_ = static_cast<Slot*>(::operator new(sizeof(Slot) * capacity_));

        for (size_t i = 0; i < capacity_; ++i) {
            new (slots_ + i) Slot();
        }

        if (old_slots) {
            for (size_t i = 0; i < old_capacity; ++i) {
                if (old_slots[i].control != Empty && old_slots[i].control != Deleted) {
                    insert(move(old_slots[i].key), move(old_slots[i].value));
                    old_slots[i].key.~K();
                    old_slots[i].value.~V();
                }
            }
            ::operator delete(old_slots);
        }
    }

    /** @brief Computes the primary bucket index for @p key.
     *  @param key The key to hash.
     *  @return The bucket index. */
    size_t hash_index(const K& key) const { return Hash{}(key) % capacity_; }

    /** @brief Computes the secondary hash fragment (h2) for @p key.
     *  @param key The key to hash.
     *  @return A 7-bit h2 value. */
    uint8_t hash_h2(const K& key) const {
        size_t h = Hash{}(key);
        uint8_t result = static_cast<uint8_t>((h >> 56) & 0x7F);
        return result == Empty || result == Deleted ? 1 : result;
    }

    Slot* slots_;              ///< Pointer to the slot array.
    size_t size_;              ///< Number of occupied slots.
    size_t capacity_;          ///< Total number of slots.
    size_t growth_threshold_;  ///< Rehash when size reaches this value.
};

}  // namespace prism
