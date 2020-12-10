#ifndef ASSEMBLER_COMPUTER_H
#define ASSEMBLER_COMPUTER_H

#include <cstddef>
#include <array>
#include <string_view>
#include <type_traits>
#include <tuple>

namespace internal {
    constexpr bool isCharacterValid(const char c) {
        return ('0' <= c && c <= '9') ||
            ('a' <= c && c <= 'z') ||
            ('A' <= c && c <= 'Z');
    }

    constexpr bool isLabelValid(std::string_view idLabel) {
        for (size_t i = 0; i < idLabel.size(); i++) {
            if (!isCharacterValid(idLabel[i])) {
                return false;
            }
        }
        return true;
    }
};

constexpr uint64_t Id(char *id) {
    std::string_view s(id);
    if (!internal::isLabelValid(s)) {
        return 0;
    }
    uint64_t codedId = 0;
    for (const auto &c : s) {
        codedId <<= 8;
        codedId += static_cast<uint8_t>(c);
    }
    return codedId;
}

template<auto V>
class Num {
public:
    static_assert(std::is_integral<decltype(V)>(), "Value is not integral type.");

    static constexpr auto getRvalue() {
        return V;
    }

private:
    static constexpr auto value = V;
};

template<typename P>
class Mem {
public:
    template<typename T, size_t N>
    static constexpr auto &getLvalue(std::array <T, N> &memory) {
        auto addr = P::template getRvalue<T, N>(memory);
        return memory[addr];
    }

    template<typename T, size_t N>
    static constexpr auto getRvalue(std::array <T, N> &memory) {
        auto addr = P::template getRvalue<T, N>(memory);
        return memory[addr];
    }
};

template<typename D, typename S>
class Mov {
public:
    template<typename T, size_t N>
    static constexpr void executeCommand(std::array <T, N> &memory) {
        D::template getLvalue<T, N>(memory) = S::template getRvalue<T, N>(memory);
    }
};

template<uint64_t T>
class Lea {};

template<uint64_t key, typename value>
class D {};

template<typename T>
struct IsLValue : public std::false_type {};

template<typename T>
struct IsLValue<Mem<T>> : public std::true_type {};

template<typename T>
struct IsRValue : public std::false_type {};

template<typename T>
struct IsRValue<Mem<T>> : public std::true_type {};

template<auto V>
struct IsRValue<Num<V>> : public std::true_type {};

// template<typename T>
// struct IsRValue<Lea<T>> : public std::true_type {};

template<typename... T>
class Program {
private:
    using Instructions = std::tuple<T...>;
public:
    // TODO: Napisać static_assert na sprawdzenie czy w programie są instrukcje -- D, Add itp.
    template<std::size_t N>
    class NthInstruction {
    public:
        using type = typename std::tuple_element<N, Instructions>::type;
    };

    template <typename, typename>
    class TupleCat;
    template <typename... First, typename... Second>
    class TupleCat<std::tuple<First...>, std::tuple<Second...>> {
        using type = std::tuple<First..., Second...>;
    };

    template<uint64_t key, size_t value>
    class KeyValuePair {};

    template<size_t index, typename... Instructions>
    struct MappingInternal {
        using type = std::tuple<>;
    };

    template<size_t index, uint64_t key, typename value, typename... Instructions> 
    struct MappingInternal<index, D<key, value>, Instructions...> {
        using type = typename TupleCat<typename MappingInternal<index + 1, Instructions...>::type, std::tuple<KeyValuePair<key, index>>>::type;
    };

    template<size_t index, typename SingleInstruction, typename... Instructions> 
    struct MappingInternal<index, SingleInstruction, Instructions...> {
        using type = typename MappingInternal<index, Instructions...>::type;
    };

    template<typename... Instructions>
    struct Mapping {
        using type = typename MappingInternal<0, Instructions...>::type;
    };

};

template<typename T>
struct IsProgram : public std::false_type {};

template<typename... T>
struct IsProgram<Program<T...>> : public std::true_type {};

template<std::size_t N, typename T>
class Computer {
public:
    static_assert(std::is_integral<T>(), "Not an integral type.");

    template<typename P>
    static constexpr std::array <T, N> boot() {
        static_assert(IsProgram<P>(), "Not a valid program type.");
        std::array <T, N> memory{};  // to zastąpić procesorem
        // Run<T, N, P>::run(memory);
        return memory;
    }
};

template <std::size_t N, typename T>
class State {
public:
    constexpr State() : State(false, false, {}) {}
    /* TODO: Dodać "ustawFlagę", "Zmień memoryBlocks" które zwrócą nowy stan */
private:
    constexpr State(bool newZf, bool newSf, std::array<T, N> newBlocks)
            : zf(newZf), sf(newSf), memoryBlocks(newBlocks) {}
    bool zf = false;
    bool sf = false;
    std::array<T, N> memoryBlocks;
};


#endif //ASSEMBLER_COMPUTER_H
