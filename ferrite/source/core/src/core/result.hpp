#pragma once

#include <variant>

#include "panic.hpp"
#include "format.hpp"


template <typename T>
struct Ok final {
    T value;

    constexpr explicit Ok(const T &t) : value(t) {}
    constexpr explicit Ok(T &&t) : value(std::move(t)) {}
};

template <typename E>
struct Err final {
    E value;

    constexpr explicit Err(const E &e) : value(e) {}
    constexpr explicit Err(E &&e) : value(std::move(e)) {}
};


template <typename T, typename E>
struct [[nodiscard]] Result final {
    std::variant<E, T> variant;

    constexpr explicit Result(const T &t) : variant(std::in_place_index<1>, t) {}
    constexpr explicit Result(const E &e) : variant(std::in_place_index<0>, e) {}
    constexpr explicit Result(T &&t) : variant(std::in_place_index<1>, std::move(t)) {}
    constexpr explicit Result(E &&e) : variant(std::in_place_index<0>, std::move(e)) {}

    constexpr Result(const Ok<T> &t) : variant(std::in_place_index<1>, std::move(t.value)) {}
    constexpr Result(const Err<E> &e) : variant(std::in_place_index<0>, std::move(e.value)) {}
    constexpr Result(Ok<T> &&t) : variant(std::in_place_index<1>, std::move(t.value)) {}
    constexpr Result(Err<E> &&e) : variant(std::in_place_index<0>, std::move(e.value)) {}

    constexpr Result(const Result &r) = default;
    constexpr Result(Result &&r) = default;
    constexpr Result &operator=(const Result &r) = default;
    constexpr Result &operator=(Result &&r) = default;

    constexpr bool is_ok() const {
        return this->variant.index() == 1;
    }
    constexpr bool is_err() const {
        return this->variant.index() == 0;
    }

    constexpr const T &ok() const {
        return std::get<1>(this->variant);
    }
    constexpr const E &err() const {
        return std::get<0>(this->variant);
    }
    constexpr T &ok() {
        return std::get<1>(this->variant);
    }
    constexpr E &err() {
        return std::get<0>(this->variant);
    }

    T expect(const std::string &message) {
        if (this->is_err()) {
            if constexpr (Printable<E>) {
                core_panic("{}: {}", message, this->err());
            } else {
                core_panic("{}", message);
            }
        }
        return std::move(this->ok());
    }
    E expect_err(const std::string &message) {
        if (this->is_ok()) {
            if constexpr (Printable<T>) {
                core_panic("{}: {}", message, this->ok());
            } else {
                core_panic("{}", message);
            }
        }
        return std::move(this->err());
    }
    T unwrap() {
        return this->expect("Result is Err");
    }
    E unwrap_err() {
        return this->expect_err("Result is Ok");
    }

    constexpr bool operator==(const Result &other) const {
        return this->variant == other.variant;
    }
    constexpr bool operator==(const Ok<T> &other) const {
        return this->is_ok() && this->ok() == other.value;
    }
    constexpr bool operator==(const Err<E> &other) const {
        return this->is_err() && this->err() == other.value;
    }
    constexpr bool operator!=(const Result &other) const {
        return this->variant != other.variant;
    }
    constexpr bool operator!=(const Ok<T> &other) const {
        return !this->is_ok() || this->ok() != other.value;
    }
    constexpr bool operator!=(const Err<E> &other) const {
        return !this->is_err() || this->err() != other.value;
    }
};

#define try_unwrap(res) \
    { \
        auto tmp = std::move(res); \
        if (tmp.is_err()) { \
            return Err(std::move(tmp.err())); \
        } \
    }

template <typename T>
std::ostream &operator<<(std::ostream &os, const Ok<T> &ok) {
    os << "Ok(";
    if constexpr (Printable<T>) {
        os << ok.value;
    }
    os << ")";
    return os;
}

template <typename E>
std::ostream &operator<<(std::ostream &os, const Err<E> &err) {
    os << "Err(";
    if constexpr (Printable<E>) {
        os << err.value;
    }
    os << ")";
    return os;
}

template <typename T, typename E>
std::ostream &operator<<(std::ostream &os, const Result<T, E> &res) {
    os << "Result::";
    if (res.is_ok()) {
        os << "Ok(";
        if constexpr (Printable<T>) {
            os << res.ok();
        }
    } else {
        os << "Err(";
        if constexpr (Printable<E>) {
            os << res.err();
        }
    }
    os << ")";
    return os;
}
