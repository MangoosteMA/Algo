/*
 * Zero based.
 * Type T must have operator += T.
 * Type T must have default constructor, which sets neutral value.
 * Operation += must be commutative.
 * For segment query type T must have operator -= T.
*/

template<typename T>
struct binary_indexed_tree {
    std::vector<T> bit;

    // Fills the array with default values.
    binary_indexed_tree(int n = 0) : bit(n + 1) {}

    // Returns size of the initial array.
    int size() const {
        return int(bit.size()) - 1;
    }

    // Adds delta at the position pos.
    void add(int pos, T delta) {
        for (pos++; pos < int(bit.size()); pos += pos & -pos)
            bit[pos] += delta;
    }

    // Returns sum on the interval [0, pref].
    T query(int pref) {
        T sum = T();
        for (pref++; pref > 0; pref -= pref & -pref)
            sum += bit[pref];

        return sum;
    }

    // Returns sum on the interval [l, r).
    T query(int l, int r) {
        if (r <= l)
            return T();

        T res = query(r - 1);
        res -= query(l - 1);
        return res;
    }
};
