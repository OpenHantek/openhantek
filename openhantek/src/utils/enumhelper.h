#pragma once

#include <QMetaEnum>
#include <QSettings>

/// Returns the c-string key of the given enum class value. Can be used for storing enum values to QSettings.
template <typename T> const char *enumName(T value) {
    return QMetaEnum::fromType<T>().valueToKey(static_cast<int>(value));
}

template <typename T> T loadForEnum(QSettings *settings, const char *key, T defaultValue) {
    bool ok = false;
    T v = static_cast<T>(QMetaEnum::fromType<T>().keyToValue(settings->value(key).toString().toUtf8().data(), &ok));
    return ok ? v : defaultValue;
}

/// Make enums declared with Q_ENUM iterable.
/// Usage: for(auto v: Enum<your_enum_type>()) {...}
template <typename T> class Enum {
  public:
    using underlying_type_t = typename std::underlying_type<T>::type;
    class Iterator {
      public:
        Iterator(underlying_type_t value) : m_value(value) {}
        T operator*(void)const { return (T)m_value; }
        void operator++(void) { ++m_value; }
        bool operator!=(Iterator rhs) { return m_value != rhs.m_value; }

      private:
        underlying_type_t m_value;
    };
};

template <typename T> typename Enum<T>::Iterator begin(Enum<T>) {
    const QMetaEnum m = QMetaEnum::fromType<T>();
    const typename Enum<T>::underlying_type_t v = m.value(0);
    return typename Enum<T>::Iterator(v);
}

template <typename T> typename Enum<T>::Iterator end(Enum<T>) {
    const QMetaEnum m = QMetaEnum::fromType<T>();
    const typename Enum<T>::underlying_type_t v = m.value(m.keyCount() - 1);
    return typename Enum<T>::Iterator(v + 1);
}
