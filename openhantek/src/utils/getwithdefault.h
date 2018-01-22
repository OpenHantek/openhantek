#pragma once
#include <iterator>
#include <map>
#include <type_traits>

template <template <class, class, class...> class C, typename K, typename V, typename... Args>
inline V &GetWithDef(const C<K, V, Args...> &m, K const &key, V &defval) {
    typename C<K, V, Args...>::const_iterator it = m.find(key);
    if (it == m.end()) return defval;
    return it->second;
}
template <template <class, class, class...> class C, typename K, typename V, typename... Args>
inline const V &GetWithDef(const C<K, V, Args...> &m, K const &key, const V &defval) {
    typename C<K, V, Args...>::const_iterator it = m.find(key);
    if (it == m.end()) return defval;
    return it->second;
}

template <typename U> static char is_shared_ptr(typename U::element_type *x);
template <typename U> static long is_shared_ptr(U *x);

/**
 * Usually, you need to iterate over std::pair entries of a map, with this class you can just iterate over the values.
 *
 * You would define begin() and end() for the const and non-const variant and then you are able to iterate over values:
 *
 * struct Abc {
 *  inline iterator begin() { return make_map_iterator(channels.begin()); }
 *  inline iterator end() { return make_map_iterator(channels.end()); }
 *
 *  inline const_iterator begin() const { return make_map_const_iterator(channels.begin()); }
 *  inline const_iterator end() const { return make_map_const_iterator(channels.end()); }
 *
 *  std::map<ChannelID, std::shared_ptr<Channel>> channels;
 * };
 *
 * And the iteration looks like this:
 *
 * Abc abc;
 * for(Channel* c: abc) do_something(c);
 *
 * As you can see, there is a special case implemented, if the value-type is a std::shared_ptr.
 */
template <typename Iter, bool isConst = false, typename Type = typename Iter::value_type::second_type,
          bool isSharedPtr = sizeof(is_shared_ptr<Type>(0)) == 1>
class map_iterator : public std::iterator<std::bidirectional_iterator_tag, Type> {
  public:
    map_iterator() {}
    map_iterator(Iter j) : i(j) {}
    map_iterator &operator++() {
        ++i;
        return *this;
    }
    map_iterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }
    map_iterator &operator--() {
        --i;
        return *this;
    }
    map_iterator operator--(int) {
        auto tmp = *this;
        --(*this);
        return tmp;
    }
    bool operator==(map_iterator j) const { return i == j.i; }
    bool operator!=(map_iterator j) const { return !(*this == j); }

    template <typename T, bool shared, bool consti> struct Helper {};

    // Shared_ptr
    template <typename T> struct Helper<T, true, false> {
        using pointer = typename Iter::value_type::second_type::element_type *;
        using reference = typename Iter::value_type::second_type::element_type *;
        static reference getRef(Iter &i) { return i->second.get(); }
        static pointer getPtr(Iter &i) { return i->second.get(); }
    };

    // Shared_ptr + const
    template <typename T> struct Helper<T, true, true> {
        using pointer = typename Iter::value_type::second_type::element_type *;
        using reference = typename Iter::value_type::second_type::element_type *;
        static const reference getRef(Iter &i) { return i->second.get(); }
        static const pointer getPtr(Iter &i) { return i->second.get(); }
    };

    template <typename T> struct Helper<T, false, false> {
        using pointer = typename Iter::value_type::second_type *;
        using reference = typename Iter::value_type::second_type &;
        static reference getRef(Iter &it) { return it->second; }
        static pointer getPtr(Iter &it) { return &it->second; }
    };

    template <typename T> struct Helper<T, false, true> {
        using pointer = typename Iter::value_type::second_type *;
        using reference = typename Iter::value_type::second_type &;
        static reference getRef(Iter &it) { return it->second; }
        static const pointer getPtr(Iter &it) { return &it->second; }
    };

    typename Helper<Type, isSharedPtr, false>::reference operator*() {
        return Helper<Type, isSharedPtr, false>::getRef(i);
    }

    typename Helper<Type, isSharedPtr, true>::reference operator*() const {
        return Helper<Type, isSharedPtr, true>::getRef(i);
    }

    typename Helper<Type, isSharedPtr, false>::pointer operator->() {
        return Helper<Type, isSharedPtr, false>::getPtr(i);
    }
    typename Helper<Type, isSharedPtr, true>::pointer operator->() const {
        return Helper<Type, isSharedPtr, true>::getPtr(i);
    }

  protected:
    Iter i;
};

template <typename Iter> inline map_iterator<Iter, false> make_map_iterator(Iter j) { return map_iterator<Iter>(j); }
template <typename Iter> inline map_iterator<Iter, true> make_map_const_iterator(Iter j) {
    return map_iterator<Iter, true>(j);
}
