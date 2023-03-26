// Copied from my previous works
// https://github.com/DmitriySud/hse-cpp-practice

#pragma once

#include <vector>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <string_view>

namespace big_numbers {

namespace {

static constexpr std::size_t CalcModule(std::size_t width) {
    return width == 1 ? 10 : 10 * CalcModule(width - 1);
}

}  // namespace

class BigInteger {
private:
    BigInteger(std::int64_t);
    BigInteger(std::uint64_t);

public:
    // Now we can use kWidth more than 9, because we use Karatsuba algorithm in
    // MultTwoCells
    static constexpr std::size_t kWidth = 16;
    static constexpr std::size_t kModule = CalcModule(kWidth);
    static_assert(kWidth % 2 == 0, "kWidth must be even for Karatsuba algorithm");

    using CellType = std::size_t;
    using ContainerType = std::vector<CellType>;

    template <typename T, class = typename std::enable_if<std::is_integral_v<T>>::type>
    BigInteger(T integer) : BigInteger(static_cast<std::int64_t>(integer)) {
    }

    BigInteger(const std::string_view&);
    BigInteger();

    BigInteger(const BigInteger&);
    BigInteger(BigInteger&&);

    BigInteger& operator=(const BigInteger&);
    BigInteger& operator=(BigInteger&&);
    BigInteger& operator=(std::int64_t);

    BigInteger& operator+=(const BigInteger&);
    BigInteger& operator-=(const BigInteger&);
    BigInteger& operator*=(const BigInteger&);
    BigInteger& operator%=(const BigInteger&);
    BigInteger& operator/=(const BigInteger&);

    BigInteger operator+(const BigInteger&) const;
    BigInteger operator-(const BigInteger&) const;
    BigInteger operator-() const;
    BigInteger operator+() const;

    BigInteger operator*(const BigInteger&) const;
    BigInteger operator/(const BigInteger&) const;
    BigInteger operator%(const BigInteger&) const;

    bool operator<(const BigInteger&) const;
    bool operator>(const BigInteger&) const;

    bool operator<=(const BigInteger&) const;
    bool operator>=(const BigInteger&) const;

    // Equals operator outside the class, because this way it will
    // have same namespace as BigInteger and be picked up by ADL
    friend bool operator==(const BigInteger&, const BigInteger&);

    std::string ToString() const;

private:
    using Less = std::less<CellType>;
    using More = std::greater<CellType>;
    using Eq = std::equal_to<CellType>;

    BigInteger(std::int8_t, ContainerType&&);

    std::int8_t sign_;
    std::vector<CellType> container_;

    BigInteger& FixSign();

    void ConstuctFromString(const std::string_view&);
};

bool operator==(const BigInteger&, const BigInteger&);

std::ostream& operator<<(std::ostream&, const big_numbers::BigInteger&);
std::istream& operator>>(std::istream&, big_numbers::BigInteger&);

}  // namespace big_numbers
