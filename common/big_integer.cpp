#include "big_integer.hpp"
#include <iomanip>
#include <sstream>
#include <cctype>
#include <cmath>

namespace big_numbers {

namespace {

using CellType = BigInteger::CellType;
using ContainerType = BigInteger::ContainerType;

CellType SumTwoCells(CellType lhs, CellType rhs, CellType& carry) {
    CellType result = (lhs + rhs) % BigInteger::kModule;
    carry += result < std::max(lhs, rhs);
    return result;
}

CellType DifTwoCells(CellType lhs, CellType rhs, CellType& carry) {
    carry += lhs < rhs;
    return lhs >= rhs ? lhs - rhs : BigInteger::kModule - (rhs - lhs);
}

CellType MultTwoCells(CellType lhs, CellType rhs, CellType& carry) {
    static constexpr CellType kModule = BigInteger::kModule;
    static constexpr CellType kHalfModule = CalcModule(BigInteger::kWidth / 2);
    CellType lhs_low = lhs % kHalfModule;
    CellType rhs_low = rhs % kHalfModule;
    CellType lhs_high = lhs / kHalfModule;
    CellType rhs_high = rhs / kHalfModule;

    CellType res_high = lhs_high * rhs_high;
    CellType res_low = lhs_low * rhs_low;

    CellType res_mid = (lhs_high + lhs_low) * (rhs_high + rhs_low) - res_high - res_low;

    res_high += res_mid / kHalfModule;
    res_low += (res_mid % kHalfModule) * kHalfModule;

    carry += res_high + res_low / kModule;
    return res_low % kModule;
}

ContainerType ModuleSum(const ContainerType& lhs, const ContainerType& rhs) {
    ContainerType result;
    CellType carry = 0;
    auto lhs_iter = lhs.begin();
    auto rhs_iter = rhs.begin();

    while (lhs_iter != lhs.end() || rhs_iter != rhs.end() || carry != 0) {
        CellType new_carry{0};
        CellType lhs_cell = lhs_iter != lhs.end() ? *lhs_iter++ : 0;
        CellType rhs_cell = rhs_iter != rhs.end() ? *rhs_iter++ : 0;

        CellType res_cell = SumTwoCells(lhs_cell, rhs_cell, new_carry);
        res_cell = SumTwoCells(res_cell, carry, new_carry);

        result.emplace_back(res_cell);
        carry = new_carry;
    }

    return result;
}

ContainerType ModuleDif(const ContainerType& lhs, const ContainerType& rhs) {
    ContainerType result;
    CellType carry = 0;
    auto lhs_iter = lhs.begin();
    auto rhs_iter = rhs.begin();

    while (lhs_iter != lhs.end() || carry != 0) {
        CellType new_carry{0};
        CellType lhs_cell = lhs_iter != lhs.end() ? *lhs_iter++ : 0;
        CellType rhs_cell = rhs_iter != rhs.end() ? *rhs_iter++ : 0;

        CellType res_cell = DifTwoCells(lhs_cell, rhs_cell, new_carry);
        res_cell = DifTwoCells(res_cell, carry, new_carry);

        result.emplace_back(res_cell);
        carry = new_carry;
    }

    while (result.size() > 1 && result[result.size() - 1] == 0) {
        result.pop_back();
    }

    return result;
}

ContainerType MultiplyOne(const ContainerType& other, CellType mult, std::size_t shift) {
    ContainerType res(shift, 0);

    CellType carry = 0;
    for (auto& item : other) {
        CellType new_carry{0};
        CellType cur = MultTwoCells(item, mult, new_carry);
        cur = SumTwoCells(cur, carry, new_carry);
        res.emplace_back(cur % BigInteger::kModule);
        carry = new_carry;
    }

    while (carry != 0) {
        res.emplace_back(carry % BigInteger::kModule);
        carry /= BigInteger::kModule;
    }

    return res;
}

template <typename Comparer>
bool AbsoluteCompare(const ContainerType& lhs, const ContainerType& rhs) {
    Comparer comp;

    if (lhs.size() != rhs.size()) {
        return comp(lhs.size(), rhs.size());
    }

    auto lhs_iter = lhs.rbegin();
    auto rhs_iter = rhs.rbegin();

    while (lhs_iter != lhs.rend() && *lhs_iter == *rhs_iter) {
        lhs_iter++;
        rhs_iter++;
    }

    return lhs_iter == lhs.rend() ? comp(1, 1) : comp(*lhs_iter, *rhs_iter);
}

void AddContainer(ContainerType& lhs, const ContainerType& rhs) {
    CellType carry = 0;
    auto lhs_iter = lhs.begin();
    auto rhs_iter = rhs.begin();

    while (lhs_iter != lhs.end() || rhs_iter != rhs.end() || carry != 0) {
        CellType new_carry{0};
        CellType lhs_cell = lhs_iter != lhs.end() ? *lhs_iter : 0;
        CellType rhs_cell = rhs_iter != rhs.end() ? *rhs_iter++ : 0;

        CellType res_cell = SumTwoCells(lhs_cell, rhs_cell, new_carry);
        res_cell = SumTwoCells(res_cell, carry, new_carry);

        if (lhs_iter == lhs.end()) {
            lhs.push_back(res_cell);
            lhs_iter = lhs.end();
        } else {
            *lhs_iter++ = res_cell;
        }
        carry = new_carry;
    }
}

void SubContainer(ContainerType& lhs, const ContainerType& rhs) {
    CellType carry = 0;
    auto lhs_iter = lhs.begin();
    auto rhs_iter = rhs.begin();

    while (lhs_iter != lhs.end() || carry != 0) {
        CellType new_carry{0};
        CellType lhs_cell = lhs_iter != lhs.end() ? *lhs_iter : 0;
        CellType rhs_cell = rhs_iter != rhs.end() ? *rhs_iter++ : 0;

        CellType res_cell = DifTwoCells(lhs_cell, rhs_cell, new_carry);
        res_cell = DifTwoCells(res_cell, carry, new_carry);

        if (lhs_iter == lhs.end()) {
            lhs.push_back(res_cell);
            lhs_iter = lhs.end();
        } else {
            *lhs_iter++ = res_cell;
        }
        carry = new_carry;
    }

    while (lhs.size() > 1 && *lhs.rbegin() == 0) {
        lhs.pop_back();
    }
}

void InvSubContainer(ContainerType& lhs, const ContainerType& rhs) {
    CellType carry = 0;
    auto lhs_iter = lhs.begin();
    auto rhs_iter = rhs.begin();

    while (rhs_iter != rhs.end() || carry != 0) {
        CellType new_carry{0};
        CellType lhs_cell = lhs_iter != lhs.end() ? *lhs_iter : 0;
        CellType rhs_cell = rhs_iter != rhs.end() ? *rhs_iter++ : 0;

        CellType res_cell = DifTwoCells(rhs_cell, lhs_cell, new_carry);
        res_cell = DifTwoCells(res_cell, carry, new_carry);

        if (lhs_iter == lhs.end()) {
            lhs.push_back(res_cell);
            lhs_iter = lhs.end();
        } else {
            *lhs_iter++ = res_cell;
        }
        carry = new_carry;
    }

    while (lhs.size() > 1 && *lhs.rbegin() == 0) {
        lhs.pop_back();
    }
}

}  // namespace

BigInteger::BigInteger() : sign_(1), container_(1) {
}

BigInteger::BigInteger(std::int64_t init_value)
    : BigInteger(static_cast<uint64_t>(std::abs(init_value))) {
    sign_ = init_value < 0 ? -1 : 1;
}

BigInteger::BigInteger(std::uint64_t init_value) : sign_(1), container_(1, init_value) {
    while (container_[container_.size() - 1] >= kModule) {
        container_.emplace_back(container_[container_.size() - 1] / kModule);
        container_[container_.size() - 2] %= kModule;
    }
}

BigInteger::BigInteger(const std::string_view& str) {
    ConstuctFromString(str);
}

void BigInteger::ConstuctFromString(const std::string_view& str) {
    container_.clear();
    sign_ = 1;

    std::size_t idx = str.size();
    while (idx - kWidth < idx) {
        container_.emplace_back(std::stoul(std::string(str.substr(idx - kWidth, kWidth))));
        idx -= kWidth;
    }

    std::string_view last = str.substr(str[0] == '-' ? 1 : 0, idx);
    if (!last.empty()) {
        container_.emplace_back(std::stoul(std::string(last)));
    }

    if (!str.empty()) {
        sign_ = str[0] == '-' ? -1 : 1;
    }
}

BigInteger::BigInteger(const BigInteger& other) : sign_(other.sign_), container_(other.container_) {
}

BigInteger::BigInteger(BigInteger&& other)
    : sign_(other.sign_), container_(std::move(other.container_)) {
}

BigInteger::BigInteger(std::int8_t sign, ContainerType&& container)
    : sign_(sign), container_(std::move(container)) {
}

BigInteger& BigInteger::operator=(const BigInteger& other) {
    sign_ = other.sign_;
    container_ = other.container_;

    return *this;
}

BigInteger& BigInteger::operator=(BigInteger&& other) {
    sign_ = other.sign_;
    other.sign_ = 0;
    container_ = std::move(other.container_);

    return *this;
}

BigInteger& BigInteger::operator=(std::int64_t other) {
    sign_ = other < 0 ? -1 : 1;
    container_.assign(1, other * sign_);
    while (container_[container_.size() - 1] >= kModule) {
        container_.emplace_back(container_[container_.size() - 1] / kModule);
        container_[container_.size() - 2] %= kModule;
    }

    return *this;
}

BigInteger& BigInteger::FixSign() {
    if (this->container_.size() == 1 && this->container_[0] == 0) {
        this->sign_ = 1;
    }
    return *this;
}

BigInteger& BigInteger::operator+=(const BigInteger& other) {
    if (sign_ == other.sign_) {
        AddContainer(container_, other.container_);
    } else if (AbsoluteCompare<More>(container_, other.container_)) {
        SubContainer(container_, other.container_);
    } else {
        sign_ = other.sign_;
        InvSubContainer(container_, other.container_);
        FixSign();
    }

    return *this;
}

BigInteger& BigInteger::operator-=(const BigInteger& other) {
    return this->operator+=(-other);
}

BigInteger BigInteger::operator+(const BigInteger& other) const {
    return BigInteger(*this) += other;
}

BigInteger BigInteger::operator-(const BigInteger& other) const {
    return BigInteger(*this) -= other;
}

BigInteger BigInteger::operator-() const {
    BigInteger copy(*this);
    copy.sign_ *= -1;

    return copy.FixSign();
}

BigInteger BigInteger::operator+() const {
    return BigInteger(*this);
}

BigInteger& BigInteger::operator*=(const BigInteger& other) {
    BigInteger copy = *this * other;
    sign_ = copy.sign_;
    container_ = std::move(copy.container_);

    return *this;
}

BigInteger BigInteger::operator*(const BigInteger& other) const {
    BigInteger res;
    res.sign_ = this->sign_ * other.sign_;
    std::size_t shift = 0;
    for (auto& item : this->container_) {
        res += BigInteger(1, MultiplyOne(other.container_, item, shift++));
    }

    return res;
}

BigInteger& BigInteger::operator/=(const BigInteger& other) {
    BigInteger res;
    BigInteger cur = *this;
    if (other == BigInteger(0)) {
        throw std::logic_error("div by zero");
    }

    while (cur >= other) {
        BigInteger mult = 1;
        while (other * mult * kModule < cur) {
            mult *= kModule;
        }

        cur -= other * mult;
        res += mult;
    }

    res.sign_ = this->sign_ * other.sign_;

    *this = std::move(res);
    return *this;
}

BigInteger BigInteger::operator/(const BigInteger& other) const {
    BigInteger copy(*this);
    return copy /= other;
}

BigInteger BigInteger::operator%(const BigInteger& other) const {
    BigInteger copy(*this);
    return copy %= other;
}

BigInteger& BigInteger::operator%=(const BigInteger& other) {
    if (other == BigInteger(0)) {
        throw std::logic_error("div by zero");
    }

    *this = *this - (*this / other) * other;
    if (*this < 0) {
        *this = +other;
    }

    return *this;
}

bool BigInteger::operator<(const BigInteger& other) const {
    bool sign_compare = sign_ == other.sign_;
    return !sign_compare && sign_ < other.sign_ ||
           sign_compare && AbsoluteCompare<Less>(container_, other.container_) == sign_ > 0;
}

bool BigInteger::operator>(const BigInteger& other) const {
    bool sign_compare = sign_ == other.sign_;
    return !sign_compare && sign_ > other.sign_ ||
           sign_compare && AbsoluteCompare<More>(container_, other.container_) == sign_ > 0;
}

bool BigInteger::operator<=(const BigInteger& other) const {
    return this->operator<(other) || *this == (other);
}

bool BigInteger::operator>=(const BigInteger& other) const {
    return this->operator>(other) || *this == (other);
}

std::string BigInteger::ToString() const {
    std::ostringstream ostream;

    if (sign_ == -1) {
        ostream << "-";
    }
    for (auto item = container_.rbegin(); item != container_.rend(); ++item) {
        if (item != container_.rbegin()) {
            ostream << std::setw(kWidth) << std::setfill('0');
        }
        ostream << *item;
    }

    return ostream.str();
}

bool operator==(const BigInteger& lhs, const BigInteger& rhs) {
    bool sign_compare = lhs.sign_ == rhs.sign_;
    return sign_compare && AbsoluteCompare<BigInteger::Eq>(lhs.container_, rhs.container_);
}

std::ostream& operator<<(std::ostream& stream, const big_numbers::BigInteger& num) {
    return stream << num.ToString();
}

std::istream& operator>>(std::istream& stream, big_numbers::BigInteger& num) {
    std::string tem;
    stream >> tem;
    num = big_numbers::BigInteger(tem);
    return stream;
}
}  // namespace big_numbers
