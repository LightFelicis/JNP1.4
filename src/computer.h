#ifndef ASSEMBLER_COMPUTER_H
#define ASSEMBLER_COMPUTER_H

#include <cstddef>
#include <array>
#include <string_view>
#include <type_traits>

class Id {
public:
    constexpr Id(char *id) : idLabel(id) {
        static_assert(isLabelValid(idLabel), "Label is not valid");
    }

private:
    static constexpr bool isCharacterValid(char c) {
        return ('0' <= c && c <= '9') ||
               ('a' <= c && c <= 'z') ||
               ('A' <= c && c <= 'Z');
    }

    static constexpr bool isLabelValid(std::string_view idLabel) {
        for (int i = 0; i < idLabel.size(); i++) {
            if (!isValidCharacter(idLabel[i])) {
                return false;
            }
        }
        return true;
    }

    std::string_view idLabel;
};

template<auto V>
class Num {
public:
    static_assert(std::is_integral<typeof(V)>(), "Value is not integral type.");

    template<typename T, size_t N>
    static constexpr auto getRvalue(std::array <T, N> &memory) {
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

template<typename T>
using IsLValue = std::false_type;

template<>
using IsLValue<Mem> = std::true_type;

template<typename T>
using IsRValue = std::false_type;

template<>
using IsRValue<Mem> = std::true_type;

template<>
using IsRValue<Num> = std::true_type;

template<>
using IsRValue<Lea> = std::true_type;

template<typename... T>
class Program {
public:
    // TODO: Napisać static_assert na sprawdzenie czy w programie są instrukcje -- D, Add itp.
    template<std::size_t N>
    class NthInstruction {
    public:
        using type = typename std::tuple_element<N, Instructions>::type;
    };

private:
    using Instructions = std::tuple<T...>;
};

template<typename T>
using IsProgram = std::false_type;

template<typename... T>
using IsProgram<Program<T...>> = std::true_type;

template<std::size_t N, typename T>
class Computer {
public:
    static_assert(std::is_integral<T>(), "Not an integral type.");

    template<typename P>
    static constexpr std::array <T, N> boot() {
        static_assert(IsProgram<P>(), "Not a valid program type.");
        std::array <T, N> memory{};  // to zastąpić procesorem
        Run<T, N, P>::run(memory);
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
