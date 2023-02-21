template<int mod>
class polynom_t : public std::vector<static_modular_int<mod>> {
public:
    using T = static_modular_int<mod>;
    using std::vector<T>::empty;
    using std::vector<T>::back;
    using std::vector<T>::pop_back;
    using std::vector<T>::size;
    using std::vector<T>::clear;
    using std::vector<T>::begin;
    using std::vector<T>::end;

private:
    static void fft(polynom_t<mod> &a);

public:
    template<typename V>
    polynom_t(const std::initializer_list<V> &lst) : std::vector<T>(lst.begin(), lst.end()) {}

    template<typename... Args>
    polynom_t(Args&&... args) : std::vector<T>(std::forward<Args>(args)...) {}

    polynom_t<mod>& resize(int n);
    void normalize();
    int degree() const; // returns -1 if all coefficients are zeroes
    T eval(const T &x) const;

    polynom_t<mod> operator-() const;
    polynom_t<mod>& operator+=(const polynom_t<mod> &another);
    polynom_t<mod>& operator-=(const polynom_t<mod> &another);
    polynom_t<mod>& operator*=(const polynom_t<mod> &another);
    polynom_t<mod>& operator/=(const polynom_t<mod> &another); // division with remainder
    polynom_t<mod>& operator%=(const polynom_t<mod> &another);

    polynom_t<mod> derivative() const;
    polynom_t<mod> integral(const T &constant = T(0)) const;
    polynom_t<mod> inv(int degree) const; // returns p^{-1} modulo x^{degree}
    polynom_t<mod> log(int degree) const; // returns log(p) modulo x^{degree}
    polynom_t<mod> exp(int degree) const; // returns exp(p) modulo x^{degree}

    template<typename V> requires (!std::is_same_v<V, polynom_t<mod>>) polynom_t<mod>& operator*=(const V &value);
    template<typename V> requires (!std::is_same_v<V, polynom_t<mod>>) polynom_t<mod>& operator/=(const V &value);
    template<int mod_> friend std::ostream& operator<<(std::ostream &out, const polynom_t<mod_> &p);
};

template<int mod>
void polynom_t<mod>::fft(polynom_t<mod> &a) {
    if (a.empty())
        return;

    static T primitive_root = T::primitive_root();
    int n = int(a.size());
    assert((n & (n - 1)) == 0);
    int lg = std::__lg(n);

    static std::vector<int> reversed_mask;
    if (int(reversed_mask.size()) != n) {
        reversed_mask.resize(n);
        for (int mask = 1; mask < n; mask++)
            reversed_mask[mask] = (reversed_mask[mask >> 1] >> 1) + ((mask & 1) << (lg - 1));
    }

    static std::vector<T> roots;
    if (int(roots.size()) != lg) {
        roots.resize(lg);
        for (int i = 0; i < lg; i++)
            roots[i] = primitive_root.power((mod - 1) / (2 << i));
    }

    for (int i = 0; i < n; i++)
        if (reversed_mask[i] < i)
            std::swap(a[i], a[reversed_mask[i]]);

    for (int len = 1; len < n; len <<= 1) {
        T root = roots[std::__lg(len)];
        for (int i = 0; i < n; i += (len << 1)) {
            T current = 1;
            for (int j = 0; j < len; j++, current *= root) {
                T value = a[i + j + len] * current;
                a[i + j + len] = a[i + j] - value;
                a[i + j] = a[i + j] + value;
            }
        }
    }
}

template<int mod>
polynom_t<mod>& polynom_t<mod>::resize(int n) {
    std::vector<T>::resize(n);
    return *this;
}

template<int mod>
void polynom_t<mod>::normalize() {
    while (!empty() && back() == T(0))
        pop_back();
}

template<int mod>
int polynom_t<mod>::degree() const {
    int deg = int(size()) - 1;
    while (deg >= 0 && (*this)[deg] == T(0))
        deg--;
    
    return deg;
}

template<int mod>
typename polynom_t<mod>::T polynom_t<mod>::eval(const T &x) const {
    T power = 1, value = 0;
    for (int i = 0; i < int(size()); i++, power *= x)
        value += (*this)[i] * power;
    
    return value;
}

template<int mod>
polynom_t<mod> polynom_t<mod>::operator-() const {
    return polynom_t(*this) * T(-1);
}

template<int mod>
polynom_t<mod>& polynom_t<mod>::operator+=(const polynom_t<mod> &another) {
    if (size() < another.size())
        resize(another.size());
    
    for (int i = 0; i < int(another.size()); i++)
        (*this)[i] += another[i];

    return *this;
}

template<int mod>
polynom_t<mod>& polynom_t<mod>::operator-=(const polynom_t<mod> &another) {
    if (size() < another.size())
        resize(another.size());
    
    for (int i = 0; i < int(another.size()); i++)
        (*this)[i] -= another[i];

    return *this;
}

template<int mod>
polynom_t<mod>& polynom_t<mod>::operator*=(const polynom_t<mod> &another) {
    if (empty() || another.empty()) {
        clear();
        return *this;
    }

    static constexpr int SIZE_MIN_CUT = 20;
    static constexpr int SIZE_MAX_CUT = 64;
    if (std::min(size(), another.size()) <= SIZE_MIN_CUT
        || std::max(size(), another.size()) <= SIZE_MAX_CUT) {
        polynom_t<mod> product(int(size() + another.size()) - 1);
        for (int i = 0; i < int(size()); i++)
            for (int j = 0; j < int(another.size()); j++)
                product[i + j] += (*this)[i] * another[j];

        product.swap(*this);
        return *this;
    }

    int real_size = int(size() + another.size()) - 1, n = 1;
    while (n < real_size)
        n <<= 1;

    resize(n);
    polynom_t b = another;
    b.resize(n);
    fft(*this), fft(b);
    for (int i = 0; i < n; i++)
        (*this)[i] *= b[i];

    fft(*this);
    std::reverse(begin() + 1, end());
    T inv_n = T(1) / T(n);
    resize(real_size);
    for (auto &x : *this)
        x *= inv_n;

    return *this;
}

template<int mod>
polynom_t<mod>& polynom_t<mod>::operator/=(const polynom_t &another) {
    polynom_t<mod> a(*this), b(another);
    a.normalize(), b.normalize();
    assert(!b.empty());
    int n = int(a.size()), m = int(b.size());
    if (n < m)
        return *this = {};

    static constexpr int N_CUT = 128;
    static constexpr int M_CUT = 64;
    if (n <= N_CUT || m <= M_CUT) {
        polynom_t<mod> quotient(n - m + 1);
        T inv_b = T(1) / b.back();
        for (int i = n - 1; i >= m - 1; i--) {
            int pos = i - m + 1;
            quotient[pos] = a[i] * inv_b;
            for (int j = 0; j < m; j++)
                a[pos + j] -= b[j] * quotient[pos];
        }
        quotient.normalize();
        return *this = quotient;
    }

    std::reverse(a.begin(), a.end());
    std::reverse(b.begin(), b.end());
    polynom_t<mod> quotient = a * b.inv(n - m + 1);
    quotient.resize(n - m + 1);
    std::reverse(quotient.begin(), quotient.end());
    quotient.normalize();
    return *this = quotient;
}

template<int mod>
polynom_t<mod>& polynom_t<mod>::operator%=(const polynom_t &another) {
    *this -= (*this) / another * another;
    normalize();
    return *this;
}

template<int mod>
polynom_t<mod> polynom_t<mod>::derivative() const {
    polynom_t<mod> der(std::max(0, int(size()) - 1));
    for (int i = 0; i < int(der.size()); i++)
        der[i] = T(i + 1) * (*this)[i + 1];
    
    return der;
}

template<int mod>
polynom_t<mod> polynom_t<mod>::integral(const T &constant) const {
    polynom_t<mod> in(size() + 1);
    in[0] = constant;
    for (int i = 1; i < int(in.size()); i++)
        in[i] = (*this)[i - 1] / T(i);
    
    return in;
}

template<int mod>
polynom_t<mod> polynom_t<mod>::inv(int degree) const {
    assert(!empty() && (*this)[0] != T(0) && "polynom is not invertable");
    static constexpr int BRUTE_FORCE_SIZE = 128;
    polynom_t<mod> inv(std::min(degree, BRUTE_FORCE_SIZE)), have(inv.size());
    T start_inv = T(1) / (*this)[0];
    for (int i = 0; i < int(inv.size()); i++) {
        inv[i] = ((i == 0 ? T(1) : T(0)) - have[i]) * start_inv;
        int steps = std::min<int>(size(), int(have.size()) - i);
        for (int j = 0; j < steps; j++)
            have[i + j] += inv[i] * (*this)[j];
    }
    for (int power = inv.size(); power < degree; power <<= 1) {
        inv *= (polynom_t<mod>({2})
            - (polynom_t<mod>(begin(), begin() + std::min<int>(size(), power << 1)) * inv).resize(power << 1));
        inv.resize(std::min(degree, power << 1));
    }
    return inv.resize(degree);
}

template<int mod>
polynom_t<mod> polynom_t<mod>::log(int degree) const {
    assert(!empty() && (*this)[0] == T(1) && "log is not defined");
    return (derivative().resize(degree) * inv(degree)).resize(degree).integral(T(0)).resize(degree);
}

template<int mod>
polynom_t<mod> polynom_t<mod>::exp(int degree) const {
    assert(!empty() && (*this)[0] == T(0) && "exp is not defined");
    polynom_t<mod> exp{1};
    for (int power = 1; power < degree; power <<= 1) {
        exp *= (polynom_t<MOD>{1} - exp.log(power << 1)
            + polynom_t<MOD>(begin(), begin() + std::min<int>(size(), power << 1)));
        exp.resize(std::min(degree, power << 1));
    }
    exp.resize(degree);
    return exp;
}

template<int mod>
template<typename V> requires (!std::is_same_v<V, polynom_t<mod>>)
polynom_t<mod>& polynom_t<mod>::operator*=(const V &value) {
    for (auto &x : *this)
        x *= value;
    
    return *this;
}

template<int mod>
template<typename V> requires (!std::is_same_v<V, polynom_t<mod>>)
polynom_t<mod>& polynom_t<mod>::operator/=(const V &value) {
    for (auto &x : *this)
        x /= value;
    
    return *this;
}

template<int mod>
std::ostream& operator<<(std::ostream &out, const polynom_t<mod> &p) {
    for (int i = 0; i < int(p.size()); i++) {
        if (i) out << ' ';
        out << p[i];
    }
    return out;
}

template<int mod>
polynom_t<mod> operator+(const polynom_t<mod> &p, const polynom_t<mod> &q) {
    return polynom_t(p) += q;
}

template<int mod>
polynom_t<mod> operator-(const polynom_t<mod> &p, const polynom_t<mod> &q) {
    return polynom_t(p) -= q;
}

template<int mod>
polynom_t<mod> operator*(const polynom_t<mod> &p, const polynom_t<mod> &q) {
    return polynom_t<mod>(p) *= q;
}

template<int mod>
polynom_t<mod> operator/(const polynom_t<mod> &p, const polynom_t<mod> &q) {
    return polynom_t<mod>(p) /= q;
}

template<int mod>
polynom_t<mod> operator%(const polynom_t<mod> &p, const polynom_t<mod> &q) {
    return polynom_t<mod>(p) %= q;
}

template<int mod, typename V> requires (!std::is_same_v<V, polynom_t<mod>>)
polynom_t<mod> operator*(const polynom_t<mod> &p, const V &value) {
    return polynom_t<mod>(p) *= value;
}

template<int mod, typename V> requires (!std::is_same_v<V, polynom_t<mod>>)
polynom_t<mod> operator*(const V &value, const polynom_t<mod> &p) {
    return polynom_t<mod>(p) *= value;
}

template<int mod, typename V> requires (!std::is_same_v<V, polynom_t<mod>>)
polynom_t<mod> operator/(const polynom_t<mod> &p, const V &value) {
    return polynom_t<mod>(p) /= value;
}

template<int mod, typename V> requires (!std::is_same_v<V, polynom_t<mod>>)
polynom_t<mod> operator/(const V &value, const polynom_t<mod> &p) {
    return polynom_t<mod>(p) /= value;
}
