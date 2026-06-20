#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace prism {

/** @brief A compile-time constant of type @p T with value @p V. */
template <typename T, T V>
struct integral_constant {
    static constexpr T value = V;    ///< The compile-time constant value.
    using value_type = T;            ///< The type of the value.
    using type = integral_constant;  ///< Self type alias.
    /** @brief Implicit conversion to the value type. */
    constexpr operator value_type() const noexcept { return value; }
};

/** @brief Integral constant type representing true. */
using true_type = integral_constant<bool, true>;
/** @brief Integral constant type representing false. */
using false_type = integral_constant<bool, false>;

// --- Type traits ---

/** @brief Trait to check if two types are the same. */
template <typename T, typename U>
struct is_same : false_type {};
/** @brief Specialization of is_same for identical types. */
template <typename T>
struct is_same<T, T> : true_type {};
/** @brief Variable template for is_same. */
template <typename T, typename U>
constexpr bool is_same_v = is_same<T, U>::value;

/** @brief Trait to remove const from a type. */
template <typename T>
struct remove_const {
    using type = T;
};
/** @brief Specialization of remove_const for const types. */
template <typename T>
struct remove_const<const T> {
    using type = T;
};
/** @brief Alias template for remove_const. */
template <typename T>
using remove_const_t = typename remove_const<T>::type;

/** @brief Trait to remove references (lvalue and rvalue) from a type. */
template <typename T>
struct remove_reference {
    using type = T;
};
/** @brief Specialization of remove_reference for lvalue references. */
template <typename T>
struct remove_reference<T&> {
    using type = T;
};
/** @brief Specialization of remove_reference for rvalue references. */
template <typename T>
struct remove_reference<T&&> {
    using type = T;
};
/** @brief Alias template for remove_reference. */
template <typename T>
using remove_reference_t = typename remove_reference<T>::type;

/** @brief Trait to remove const and volatile qualifiers from a type. */
template <typename T>
struct remove_cv {
    using type = T;
};
/** @brief Specialization of remove_cv for const types. */
template <typename T>
struct remove_cv<const T> {
    using type = T;
};
/** @brief Specialization of remove_cv for volatile types. */
template <typename T>
struct remove_cv<volatile T> {
    using type = T;
};
/** @brief Specialization of remove_cv for const volatile types. */
template <typename T>
struct remove_cv<const volatile T> {
    using type = T;
};
/** @brief Alias template for remove_cv. */
template <typename T>
using remove_cv_t = typename remove_cv<T>::type;

/** @brief Trait that adds const to a type. */
template <typename T>
struct add_const {
    using type = const T;
};
/** @brief Alias template for add_const. */
template <typename T>
using add_const_t = typename add_const<T>::type;

/** @brief Trait that adds a pointer to a type. */
template <typename T>
struct add_pointer {
    using type = T*;
};
/** @brief Alias template for add_pointer. */
template <typename T>
using add_pointer_t = typename add_pointer<T>::type;

/** @brief Trait that decays a type by removing references and cv-qualifiers. */
template <typename T>
struct decay {
    using type = remove_cv_t<remove_reference_t<T>>;
};
/** @brief Alias template for decay. */
template <typename T>
using decay_t = typename decay<T>::type;

/** @brief SFINAE helper that provides a member type @c type only when @p B is true. */
template <bool B, typename T = void>
struct enable_if {};
/** @brief Specialization of enable_if when @p B is true. */
template <typename T>
struct enable_if<true, T> {
    using type = T;
};
/** @brief Alias template for enable_if. */
template <bool B, typename T = void>
using enable_if_t = typename enable_if<B, T>::type;

/** @brief Selects type @p T if @p B is true, otherwise type @p F. */
template <bool B, typename T, typename F>
struct conditional {
    using type = T;
};
/** @brief Specialization of conditional when @p B is false. */
template <typename T, typename F>
struct conditional<false, T, F> {
    using type = F;
};
/** @brief Alias template for conditional. */
template <bool B, typename T, typename F>
using conditional_t = typename conditional<B, T, F>::type;

/** @brief Variable template that is true if @p T is an integral type. */
template <typename T>
constexpr bool is_integral_v = std::is_integral_v<T>;
/** @brief Variable template that is true if @p T is a floating-point type. */
template <typename T>
constexpr bool is_floating_point_v = std::is_floating_point_v<T>;
/** @brief Variable template that is true if @p T is an arithmetic type (integral or floating-point). */
template <typename T>
constexpr bool is_arithmetic_v = is_integral_v<T> || is_floating_point_v<T>;
/** @brief Variable template that is true if @p T is a pointer type. */
template <typename T>
constexpr bool is_pointer_v = std::is_pointer_v<T>;
/** @brief Variable template that is true if @p T is a class/struct/union type. */
template <typename T>
constexpr bool is_class_v = std::is_class_v<T>;
/** @brief Variable template that is true if @p T is an enumeration type. */
template <typename T>
constexpr bool is_enum_v = std::is_enum_v<T>;
/** @brief Variable template that is true if @p T is void (after removing cv-qualifiers). */
template <typename T>
constexpr bool is_void_v = is_same_v<remove_cv_t<T>, void>;
/** @brief Variable template that is true if @p T is const-qualified. */
template <typename T>
constexpr bool is_const_v = std::is_const_v<T>;
/** @brief Variable template that is true if @p T is trivially copyable. */
template <typename T>
constexpr bool is_trivially_copyable_v = std::is_trivially_copyable_v<T>;
/** @brief Variable template that is true if @p T is trivially destructible. */
template <typename T>
constexpr bool is_trivially_destructible_v = std::is_trivially_destructible_v<T>;

/** @brief Variable template that is true if @p Base is a base class of @p Derived. */
template <typename Base, typename Derived>
constexpr bool is_base_of_v = std::is_base_of_v<Base, Derived>;

/** @brief Logical AND of all type trait templates in @p Bs. */
template <typename...>
struct conjunction : true_type {};
/** @brief Base case for conjunction. */
template <typename B>
struct conjunction<B> : B {};
/** @brief Recursive conjunction: true if all @p B, @p Bs... are true. */
template <typename B, typename... Bs>
struct conjunction<B, Bs...> : conditional_t<bool(B::value), conjunction<Bs...>, B> {};
/** @brief Variable template for conjunction. */
template <typename... Bs>
constexpr bool conjunction_v = conjunction<Bs...>::value;

/** @brief Logical OR of all type trait templates in @p Bs. */
template <typename...>
struct disjunction : false_type {};
/** @brief Base case for disjunction. */
template <typename B>
struct disjunction<B> : B {};
/** @brief Recursive disjunction: true if any of @p B, @p Bs... is true. */
template <typename B, typename... Bs>
struct disjunction<B, Bs...> : conditional_t<bool(B::value), B, disjunction<Bs...>> {};
/** @brief Variable template for disjunction. */
template <typename... Bs>
constexpr bool disjunction_v = disjunction<Bs...>::value;

/** @brief Logical negation of a type trait. */
template <typename B>
struct negation : integral_constant<bool, !bool(B::value)> {};
/** @brief Variable template for negation. */
template <typename B>
constexpr bool negation_v = negation<B>::value;

// --- Type trait helpers ---

/** @brief Trait to detect lvalue references. */
template <typename T>
struct is_lvalue_reference : false_type {};
/** @brief Specialization for lvalue references. */
template <typename T>
struct is_lvalue_reference<T&> : true_type {};
/** @brief Variable template for is_lvalue_reference. */
template <typename T>
constexpr bool is_lvalue_reference_v = is_lvalue_reference<T>::value;

/** @brief Trait to detect rvalue references. */
template <typename T>
struct is_rvalue_reference : false_type {};
/** @brief Specialization for rvalue references. */
template <typename T>
struct is_rvalue_reference<T&&> : true_type {};
/** @brief Variable template for is_rvalue_reference. */
template <typename T>
constexpr bool is_rvalue_reference_v = is_rvalue_reference<T>::value;

// --- Move / Forward ---

/** @brief Casts an lvalue to an rvalue reference to enable move semantics.
 *  @param t The value to cast.
 *  @return An rvalue reference to @p t. */
template <typename T>
constexpr remove_reference_t<T>&& move(T& t) noexcept {
    return static_cast<remove_reference_t<T>&&>(t);
}

/** @brief Forwards an lvalue as an lvalue or rvalue reference (lvalue overload).
 *  @param t The lvalue to forward.
 *  @return An lvalue or rvalue reference preserving the value category. */
template <typename T>
constexpr T&& forward(remove_reference_t<T>& t) noexcept {
    return static_cast<T&&>(t);
}

/** @brief Forwards an rvalue as an rvalue reference (rvalue overload).
 *  @param t The rvalue to forward.
 *  @return An rvalue reference preserving the value category. */
template <typename T>
constexpr T&& forward(remove_reference_t<T>&& t) noexcept {
    static_assert(!is_lvalue_reference_v<T>, "cannot forward an rvalue as an lvalue");
    return static_cast<T&&>(t);
}

/** @brief Returns a const reference to @p t without making a copy.
 *  @param t The value to make const.
 *  @return A const reference to @p t. */
template <typename T>
constexpr const T& as_const(T& t) noexcept {
    return t;
}

// --- Swap ---

/** @brief Swaps the values of @p a and @p b using move semantics.
 *  @param a The first value.
 *  @param b The second value. */
template <typename T>
constexpr void swap(T& a, T& b) noexcept {
    T temp = move(a);
    a = move(b);
    b = move(temp);
}

// --- Pair ---

/** @brief A simple pair container holding two values of types @p T1 and @p T2. */
template <typename T1, typename T2>
struct Pair {
    T1 first;   ///< The first element.
    T2 second;  ///< The second element.

    /** @brief Default constructor. */
    constexpr Pair() = default;
    /** @brief Constructs a Pair from two values.
     *  @param f The first value.
     *  @param s The second value. */
    constexpr Pair(const T1& f, const T2& s) : first(f), second(s) {}
    /** @brief Constructs a Pair from two moved values.
     *  @param f The first value to move.
     *  @param s The second value to move. */
    constexpr Pair(T1&& f, T2&& s) : first(move(f)), second(move(s)) {}
    /** @brief Copy constructor. */
    constexpr Pair(const Pair&) = default;
    /** @brief Move constructor. */
    constexpr Pair(Pair&&) = default;
    /** @brief Copy assignment. */
    constexpr Pair& operator=(const Pair&) = default;
    /** @brief Move assignment. */
    constexpr Pair& operator=(Pair&&) = default;
    /** @brief Equality comparison.
     *  @param o The Pair to compare.
     *  @return True if both elements are equal. */
    constexpr bool operator==(const Pair& o) const { return first == o.first && second == o.second; }
    /** @brief Less-than comparison (lexicographic).
     *  @param o The Pair to compare.
     *  @return True if this Pair is lexicographically less than @p o. */
    constexpr bool operator<(const Pair& o) const { return first < o.first || (first == o.first && second < o.second); }
};

/** @brief Creates a Pair with template argument deduction.
 *  @param first The first element.
 *  @param second The second element.
 *  @return A Pair containing the given values. */
template <typename T1, typename T2>
constexpr Pair<T1, T2> make_pair(T1&& first, T2&& second) {
    return Pair<T1, T2>(forward<T1>(first), forward<T2>(second));
}

// --- Integer sequence ---

/** @brief A compile-time sequence of integer values of type @p T. */
template <typename T, T... Ints>
struct integer_sequence {
    using value_type = T;  ///< The integer type.
    /** @brief Returns the number of integers in the sequence. */
    static constexpr size_t size() { return sizeof...(Ints); }
};

/** @brief An integer_sequence of size_t values. */
template <size_t... Ints>
using index_sequence = integer_sequence<size_t, Ints...>;

/** @brief Helper to recursively generate an index_sequence from 0 to N-1. */
template <size_t N, size_t... Is>
struct make_index_sequence_helper : make_index_sequence_helper<N - 1, N - 1, Is...> {};

/** @brief Base case for make_index_sequence_helper when N is 0. */
template <size_t... Is>
struct make_index_sequence_helper<0, Is...> {
    using type = index_sequence<Is...>;
};

/** @brief Alias template that generates an index_sequence from 0 to N-1. */
template <size_t N>
using make_index_sequence = typename make_index_sequence_helper<N>::type;

// --- bit_cast ---

/** @brief Reinterprets the object representation of @p from as type @p To.
 *  @tparam To The destination type.
 *  @tparam From The source type.
 *  @param from The value to bit-cast.
 *  @return The bit-cast result. */
template <typename To, typename From>
constexpr To bit_cast(const From& from) noexcept {
    static_assert(sizeof(To) == sizeof(From), "bit_cast requires equal sizes");
    static_assert(is_trivially_copyable_v<To>, "To must be trivially copyable");
    static_assert(is_trivially_copyable_v<From>, "From must be trivially copyable");
    To to;
    __builtin_memcpy(&to, &from, sizeof(To));
    return to;
}

}  // namespace prism
