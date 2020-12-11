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

constexpr uint64_t Id(const char *id) {
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
struct Num {
public:
    static_assert(std::is_integral<decltype(V)>(), "Value is not integral type.");

    static constexpr auto getRvalue() {
        return V;
    }

private:
    static constexpr auto value = V;
};

template<typename P>
struct Mem {
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
struct Mov {
public:
    template<typename T, size_t N>
    static constexpr void executeCommand(std::array <T, N> &memory) {
        D::template getLvalue<T, N>(memory) = S::template getRvalue<T, N>(memory);
    }
};

template<uint64_t>
struct Label {};

template<typename T>
struct Jmp {};

template<uint64_t T>
struct Lea {};

template<uint64_t key, typename value>
struct D {};

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

template<uint64_t V>
struct IsRValue<Lea<V>> : public std::true_type {};

template<typename... T>
struct Program {
    using Instructions = std::tuple<T...>;

    // TODO: Napisać static_assert na sprawdzenie czy w programie są instrukcje -- D, Add itp.
    template<std::size_t N>
    struct NthInstruction {
    public:
        using type = typename std::tuple_element<N, Instructions>::type;
    };

    template <typename, typename>
    struct TupleCat;
    template <typename... First, typename... Second>
    struct TupleCat<std::tuple<First...>, std::tuple<Second...>> {
        using type = std::tuple<First..., Second...>;
    };

    template<uint64_t key, size_t value>
    struct KeyValuePair {};

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

    template<uint64_t key, typename MappingType>
    struct MatchKeyMapping {};

    template <uint64_t key, size_t val, typename ...OtherKeyValuePairs>
    struct MatchKeyMapping<key, std::tuple<KeyValuePair<key, val>, OtherKeyValuePairs...>> {
        constexpr static size_t value = val;
    };

    template <uint64_t key, uint64_t key2, size_t val, typename ...OtherKeyValuePairs>
    struct MatchKeyMapping<key, std::tuple<KeyValuePair<key2, val>, OtherKeyValuePairs...>> {
        constexpr static size_t value = MatchKeyMapping<key, std::tuple<OtherKeyValuePairs...>>::value;
    };

};

template<typename T>
struct IsProgram : public std::false_type {};

template<typename... T>
struct IsProgram<Program<T...>> : public std::true_type {};

template <std::size_t memorySize, typename T>
struct State {
    constexpr State() : State(false, false, {}) {}
    bool zf = false;
    bool sf = false;
    std::array<T, memorySize> memoryBlocks;
};

/* Initial parsing operations */
// Tu wyszukuję tylko polecenia D, zeby zaktualizować memory, pozostałe powinny nie modyfikować memory
template<size_t memorySize, typename T, typename InitialMapping, typename... Instructions>
struct InitialInstructionsParsing {
    constexpr static void evaluate(State<memorySize, T> &) {}
};

template<size_t memorySize, typename T, uint64_t key, typename value, typename InitialMapping, typename... Instructions>
struct InitialInstructionsParsing <memorySize, T, InitialMapping, std::tuple<D<key, value>, Instructions...>> {
    constexpr static void evaluate(State<memorySize, T> &s) {
        constexpr std::size_t position = Program<>::MatchKeyMapping<key, std::tuple<>>::value;
        s[position] = value::getRvalue();
        InitialInstructionsParsing<memorySize, T, InitialMapping, Instructions...>::evaluate(s);        
    }
};

template<size_t memorySize, typename T, typename InitialMapping, typename SingleInstruction, typename... Instructions>
struct InitialInstructionsParsing <memorySize, T, InitialMapping, std::tuple<SingleInstruction, Instructions...>> {
    constexpr static void evaluate(State<memorySize, T> &s) {
        InitialInstructionsParsing<memorySize, T, InitialMapping, Instructions...>::evaluate(s);
    }
};

// Pamięć została zmodyfikowana tak, zeby odpowiednie deklaracje były powpisywane w pamięć. Teraz "właściwe" przechodzenie po instrukcjach.
template<size_t memorySize, typename T, typename InitialMapping, typename JumpLabel, bool passedLabel, typename InstructionsOrigin, typename... Instructions>
struct InstructionsRunner {
    constexpr static void evaluate(State<memorySize, T> &) {}
};

template<size_t memorySize, typename T, typename InitialMapping, uint64_t keyLabel, uint64_t keyLabel2, typename InstructionsOrigin, typename... Instructions>
struct InstructionsRunner <memorySize, T, InitialMapping, Label<keyLabel>, false, InstructionsOrigin, Label<keyLabel2>, Instructions...> {
    constexpr static void evaluate(State<memorySize, T> &s) {
        InstructionsRunner<memorySize, T, InitialMapping, Label<keyLabel>, keyLabel == keyLabel2, InstructionsOrigin, Instructions...>::evaluate(s);
    }
};

template<size_t memorySize, typename T, typename InitialMapping, typename keyLabel, typename InstructionsOrigin, typename SingleInstruction, typename... Instructions>
struct InstructionsRunner <memorySize, T, InitialMapping, keyLabel, false, InstructionsOrigin, SingleInstruction, Instructions...> {
    constexpr static void evaluate(State<memorySize, T> &s) {
        InstructionsRunner<memorySize, T, InitialMapping, keyLabel, false, InstructionsOrigin, Instructions...>::evaluate(s);
    }
};

// Wywołanie rekurencyjne, jak mamy Jmp to od początku tylko z tą nową labelką i flagą na false.
template<size_t memorySize, typename T, typename InitialMapping, typename keyLabel, typename newLabel, typename... InstructionsOrigin, typename... Instructions>
struct InstructionsRunner <memorySize, T, InitialMapping, keyLabel, true, std::tuple<InstructionsOrigin...>, Jmp<newLabel>, Instructions...> {
    constexpr static void evaluate(State<memorySize, T> &s) {
        InstructionsRunner<memorySize, T, InitialMapping, newLabel, false, std::tuple<InstructionsOrigin...>, InstructionsOrigin...>::evaluate(s);
    }
};

template<size_t memorySize, typename T, typename InitialMapping, typename keyLabel, typename SingleInstruction, typename InstructionsOrigin, typename... Instructions>
struct InstructionsRunner <memorySize, T, InitialMapping, keyLabel, true, InstructionsOrigin, std::tuple<SingleInstruction, Instructions...>> {
    constexpr static void evaluate(State<memorySize, T> &s) {
        // TODO: Dodać pozostałe polecenia czyli Label, Inc, Dec itp.
        (void) s;
    }
};


template<std::size_t memorySize, typename T>
struct Computer {
public:
    static_assert(std::is_integral<T>(), "Not an integral type.");

    template<typename... ProgramInstructions>
    static constexpr std::array <T, memorySize> boot() {
        static_assert(IsProgram<ProgramInstructions...>(), "Not a valid program type.");
        State<memorySize, T> computerMemory; // Tworzę pamięć dla komputera + flagi
        // Run<T, memorySize, ProgramInstructions...>::run(computerMemory);
        return computerMemory.memoryBlocks;
    }
};

#endif //ASSEMBLER_COMPUTER_H
