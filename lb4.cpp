#include <iostream>
#include <cstring>
#include <stdexcept>
#include <functional>
#include <sstream>
#include <type_traits>
#include <cctype>
#include <algorithm>

// Винятки 
class StringException : public std::exception {
    std::string message;
public:
    explicit StringException(const std::string& msg) : message("String error: " + msg) {}
    const char* what() const noexcept override { return message.c_str(); }
};

class OutOfRangeException : public StringException {
public:
    explicit OutOfRangeException(size_t index)
        : StringException("Index out of range: " + std::to_string(index)) {}
};

class InvalidRangeException : public StringException {
public:
    InvalidRangeException() : StringException("Invalid pointer range.") {}
};

// Абстрактна трансформація
template <typename T>
struct Transformer {
    virtual T operator()(const T&) const = 0;
    virtual ~Transformer() = default;
};

// Клас String<T> 
template <typename T>
class String {
    T* data = nullptr;
    size_t length = 0;

    void copy_from(const T* source, size_t len) {
        data = new T[len];
        for (size_t i = 0; i < len; ++i) data[i] = source[i];
        length = len;
    }

public:
    // Конструктори
    String() = default;

    String(const String& other) {
        copy_from(other.data, other.length);
    }

    String(String&& other) noexcept : data(other.data), length(other.length) {
        other.data = nullptr;
        other.length = 0;
    }

    String(size_t count, const T& ch) {
        data = new T[count];
        for (size_t i = 0; i < count; ++i) data[i] = ch;
        length = count;
    }

    String(const T* cstr) {
        if constexpr (std::is_same<T, char>::value) {
            length = std::strlen(cstr);
        } else {
            length = 0;
            while (!(cstr[length] == T())) ++length;
        }
        copy_from(cstr, length);
    }

    String(const T* begin, const T* end) {
        if (begin > end) throw InvalidRangeException();
        size_t len = end - begin;
        copy_from(begin, len);
    }

    template <typename U>
    String(const String<U>& other) {
        length = other.size();
        data = new T[length];
        for (size_t i = 0; i < length; ++i) data[i] = static_cast<T>(other[i]);
    }

    ~String() { delete[] data; }

    // Присвоєння
    String& operator=(const String& other) {
        if (this != &other) {
            delete[] data;
            copy_from(other.data, other.length);
        }
        return *this;
    }

    String& operator=(String&& other) noexcept {
        if (this != &other) {
            delete[] data;
            data = other.data;
            length = other.length;
            other.data = nullptr;
            other.length = 0;
        }
        return *this;
    }

    // Методи доступу
    size_t size() const { return length; }
    bool empty() const { return length == 0; }
    void clear() {
        delete[] data;
        data = nullptr;
        length = 0;
    }

    T& operator[](size_t index) {
        if (index >= length) throw OutOfRangeException(index);
        return data[index];
    }

    const T& operator[](size_t index) const {
        if (index >= length) throw OutOfRangeException(index);
        return data[index];
    }

    String substr(size_t start, size_t len) const {
        if (start > length) throw OutOfRangeException(start);
        size_t actualLen = std::min(len, length - start);
        return String(data + start, data + start + actualLen);
    }

    // Оператори конкатенації
    String operator+(const String& other) const {
        String result;
        result.data = new T[length + other.length];
        for (size_t i = 0; i < length; ++i) result.data[i] = data[i];
        for (size_t i = 0; i < other.length; ++i) result.data[length + i] = other.data[i];
        result.length = length + other.length;
        return result;
    }

    String& operator+=(const T& ch) {
        T* newData = new T[length + 1];
        for (size_t i = 0; i < length; ++i) newData[i] = data[i];
        newData[length] = ch;
        delete[] data;
        data = newData;
        ++length;
        return *this;
    }

    // Трансформації
    void apply(const Transformer<T>& transformer) {
        for (size_t i = 0; i < length; ++i) data[i] = transformer(data[i]);
    }

    template <typename Trans>
    void modify(const Trans& transformer) {
        for (size_t i = 0; i < length; ++i) data[i] = transformer(data[i]);
    }

    template <typename Trans>
    String transformed(const Trans& transformer) const {
        String result;
        result.data = new T[length];
        for (size_t i = 0; i < length; ++i) result.data[i] = transformer(data[i]);
        result.length = length;
        return result;
    }

    String transformed(const Transformer<T>& transformer) const {
        return transformed<T>([&transformer](const T& c) { return transformer(c); });
    }

    // Доступ до data (для зовнішніх операторів)
    const T* c_str() const { return data ? data : ""; }

    // Друзі для операторів
    template <typename U>
    friend String<U> operator+(const String<U>& s, const U& ch);

    template <typename U>
    friend String<U> operator+(const U& ch, const String<U>& s);

    template <typename U>
    friend String<U> operator*(const String<U>& s, int times);

    template <typename U>
    friend String<U> operator*(int times, const String<U>& s);

    template <typename U>
    friend bool operator==(const String<U>& a, const String<U>& b);

    template <typename U>
    friend bool operator!=(const String<U>& a, const String<U>& b);

    template <typename U>
    friend bool operator<(const String<U>& a, const String<U>& b);

    template <typename U>
    friend bool operator>(const String<U>& a, const String<U>& b);

    template <typename U>
    friend bool operator<=(const String<U>& a, const String<U>& b);

    template <typename U>
    friend bool operator>=(const String<U>& a, const String<U>& b);

    // Оператори вводу/виводу оголошуємо друзями (тільки для char)
    friend std::ostream& operator<<(std::ostream& os, const String<char>& str);
    friend std::istream& operator>>(std::istream& is, String<char>& str);
};

// Реалізація друкованих і ввідних операторів поза класом

std::ostream& operator<<(std::ostream& os, const String<char>& str) {
    for (size_t i = 0; i < str.size(); ++i) {
        os << str[i];
    }
    return os;
}

std::istream& operator>>(std::istream& is, String<char>& str) {
    std::string temp;
    is >> temp;
    str = String<char>(temp.c_str());
    return is;
}

// Оператори конкатенації та повторення

template <typename T>
String<T> operator+(const String<T>& s, const T& ch) {
    String<T> result = s;
    result += ch;
    return result;
}

template <typename T>
String<T> operator+(const T& ch, const String<T>& s) {
    String<T> result(1, ch);
    return result + s;
}

template <typename T>
String<T> operator*(const String<T>& s, int times) {
    if (times <= 0) return String<T>();
    String<T> result;
    result.data = new T[s.length * times];
    for (int t = 0; t < times; ++t)
        for (size_t i = 0; i < s.length; ++i)
            result.data[t * s.length + i] = s.data[i];
    result.length = s.length * times;
    return result;
}

template <typename T>
String<T> operator*(int times, const String<T>& s) {
    return s * times;
}

// Порівняльні оператори

template <typename T>
bool operator==(const String<T>& a, const String<T>& b) {
    if (a.length != b.length) return false;
    for (size_t i = 0; i < a.length; ++i)
        if (a.data[i] != b.data[i]) return false;
    return true;
}

template <typename T>
bool operator!=(const String<T>& a, const String<T>& b) {
    return !(a == b);
}

template <typename T>
bool operator<(const String<T>& a, const String<T>& b) {
    size_t min_len = std::min(a.length, b.length);
    for (size_t i = 0; i < min_len; ++i) {
        if (a.data[i] < b.data[i]) return true;
        if (a.data[i] > b.data[i]) return false;
    }
    return a.length < b.length;
}

template <typename T>
bool operator>(const String<T>& a, const String<T>& b) {
    return b < a;
}

template <typename T>
bool operator<=(const String<T>& a, const String<T>& b) {
    return !(a > b);
}

template <typename T>
bool operator>=(const String<T>& a, const String<T>& b) {
    return !(a < b);
}

// Приклад: перетворювач для зміни символів на верхній регістр
struct ToUpperChar : Transformer<char> {
    char operator()(const char& c) const override {
        return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
};

// Головна функція з меню

void printMenu() {
    std::cout << "\n=== Меню для String<char> ===\n"
              << "1. Ввести рядок\n"
              << "2. Вивести рядок\n"
              << "3. Довжина рядка\n"
              << "4. Доступ до символу за індексом\n"
              << "5. Взяти підрядок\n"
              << "6. Додати символ в кінець\n"
              << "7. Конкатенація з іншим рядком\n"
              << "8. Помножити рядок на число\n"
              << "9. Застосувати перетворення ToUpperChar\n"
              << "0. Вийти\n"
              << "Виберіть опцію: ";
}

int main() {
    String<char> s;
    bool running = true;

    while (running) {
        printMenu();
        int choice;
        std::cin >> choice;

        try {
            switch (choice) {
            case 1: {
                std::cout << "Введіть рядок: ";
                std::cin >> s;
                break;
            }
            case 2: {
                std::cout << "Поточний рядок: " << s << '\n';
                break;
            }
            case 3: {
                std::cout << "Довжина рядка: " << s.size() << '\n';
                break;
            }
            case 4: {
                std::cout << "Введіть індекс: ";
                size_t idx; std::cin >> idx;
                std::cout << "Символ за індексом " << idx << ": " << s[idx] << '\n';
                break;
            }
            case 5: {
                std::cout << "Введіть початковий індекс і довжину підрядка: ";
                size_t start, len;
                std::cin >> start >> len;
                String<char> sub = s.substr(start, len);
                std::cout << "Підрядок: " << sub << '\n';
                break;
            }
            case 6: {
                std::cout << "Введіть символ для додавання в кінець: ";
                char ch; std::cin >> ch;
                s += ch;
                std::cout << "Після додавання: " << s << '\n';
                break;
            }
            case 7: {
                std::cout << "Введіть рядок для конкатенації: ";
                String<char> other; std::cin >> other;
                s = s + other;
                std::cout << "Після конкатенації: " << s << '\n';
                break;
            }
            case 8: {
                std::cout << "Введіть кількість повторень: ";
                int times; std::cin >> times;
                s = s * times;
                std::cout << "Після множення: " << s << '\n';
                break;
            }
            case 9: {
                ToUpperChar toUpper;
                s.apply(toUpper);
                std::cout << "Після перетворення в верхній регістр: " << s << '\n';
                break;
            }
            case 0:
                running = false;
                break;
            default:
                std::cout << "Невірна опція.\n";
                break;
            }
        }
        catch (const StringException& ex) {
            std::cerr << "Помилка: " << ex.what() << '\n';
        }
        catch (const std::exception& ex) {
            std::cerr << "Стандартна помилка: " << ex.what() << '\n';
        }
    }

    std::cout << "Програма завершена.\n";
    return 0;
}
