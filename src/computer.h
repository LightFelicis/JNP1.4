#ifndef ASSEMBLER_COMPUTER_H
#define ASSEMBLER_COMPUTER_H

#include <array>
#include <cstddef>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace internal {
constexpr bool isCharacterValid(const char c) {
  return ('0' <= c && c <= '9') || ('a' <= c && c <= 'z') ||
         ('A' <= c && c <= 'Z');
}

constexpr bool isLabelValid(std::string_view idLabel) {
  if (idLabel.size() == 0 || idLabel.size() > 6) {
    return false;
  }
  for (size_t i = 0; i < idLabel.size(); i++) {
    if (!isCharacterValid(idLabel[i])) {
      return false;
    }
  }
  return true;
}
};

template <std::size_t memorySize, typename T>
struct State {
  constexpr State()
      : zf(false),
        sf(false),
        memoryBlocks(std::array<T, memorySize>()),
        declarationIDs(std::array<uint64_t, memorySize>()),
        decCount(0) {}

  bool zf = false;
  bool sf = false;
  std::array<T, memorySize> memoryBlocks;
  std::array<uint64_t, memorySize> declarationIDs;
  size_t decCount;
};

constexpr uint64_t Id(const char *id) {
  std::string_view s(id);
  if (!internal::isLabelValid(s)) {
    throw std::invalid_argument("Invalid Id.");
  }
  uint64_t codedId = 0;
  for (const auto &c : s) {
    codedId <<= 8;
    if (static_cast<uint8_t>(c) >= 'a') {
      codedId += static_cast<uint8_t>(c - 'a');
    } else {
      codedId += static_cast<uint8_t>(c - 'A');
    }
  }
  return codedId;
}

template <auto V>
struct Num {
  static_assert(std::is_integral<decltype(V)>(), "Value is not integral type.");

  template <typename T, size_t memorySize>
  static constexpr auto getRvalue(State<memorySize, T> &) {
    return V;
  }

  static constexpr auto value = V;
};

template <typename P>
struct Mem {
  template <typename T, size_t memorySize>
  static constexpr auto &getLvalue(State<memorySize, T> &s) {
    auto addr = P::template getRvalue<T, memorySize>(s);
    return s.memoryBlocks[addr];
  }

  template <typename T, size_t memorySize>
  static constexpr auto getRvalue(State<memorySize, T> &s) {
    auto addr = P::template getRvalue<T, memorySize>(s);
    return s.memoryBlocks[addr];
  }
};

template <uint64_t I>
struct Lea {
  template <typename T, size_t memorySize>
  static constexpr auto getRvalue(State<memorySize, T> &s) {
    for (size_t i = 0; i < s.decCount; i++) {
      if (I == s.declarationIDs[i]) return i;
    }
    throw std::invalid_argument("Nonexisting ID");
  }
};

/* Instrukcje */

template <typename Dsc, typename Src>
struct Mov {};

template <uint64_t>
struct Label {};

template <uint64_t T>
struct Jmp {};

template <uint64_t T>
struct Jz {};

template <uint64_t T>
struct Js {};

template <uint64_t key, typename value>
struct D {};

/* Arytmetyka */

template <typename Arg1, typename Arg2>
struct Add {};

template <typename Arg1, typename Arg2>
struct Sub {};

template <typename Arg>
struct Inc {};

template <typename Arg>
struct Dec {};

/* Operacje logiczne */

template <typename Arg1, typename Arg2>
struct And {};

template <typename Arg1, typename Arg2>
struct Or {};

template <typename Arg>
struct Not {};

/* Operacja porownania */

template <typename Arg1, typename Arg2>
struct Cmp {};

/* Specjalizacje, ktore wartości mogą być L/R-value */

template <typename T>
struct IsLValue : public std::false_type {};

template <typename T>
struct IsLValue<Mem<T>> : public std::true_type {};

template <typename T>
struct IsRValue : public std::false_type {};

template <typename T>
struct IsRValue<Mem<T>> : public std::true_type {};

template <auto V>
struct IsRValue<Num<V>> : public std::true_type {};

template <uint64_t I>
struct IsRValue<Lea<I>> : public std::true_type {};

/* Co jest poprawną instrukcją */

template <typename T>
struct isProperInstruction : public std::false_type {};

template <typename Arg1, typename Arg2>
struct isProperInstruction<Add<Arg1, Arg2>> : public std::true_type {};

template <typename Arg1, typename Arg2>
struct isProperInstruction<Sub<Arg1, Arg2>> : public std::true_type {};

template <typename Arg>
struct isProperInstruction<Inc<Arg>> : public std::true_type {};

template <typename Arg>
struct isProperInstruction<Dec<Arg>> : public std::true_type {};

template <uint64_t Key, typename Value>
struct isProperInstruction<D<Key, Value>> : public std::true_type {};

template <typename Dsc, typename Src>
struct isProperInstruction<Mov<Dsc, Src>> : public std::true_type {};

template <uint64_t V>
struct isProperInstruction<Label<V>> : public std::true_type {};

template <uint64_t T>
struct isProperInstruction<Jmp<T>> : public std::true_type {};

template <uint64_t T>
struct isProperInstruction<Jz<T>> : public std::true_type {};

template <uint64_t T>
struct isProperInstruction<Js<T>> : public std::true_type {};

template <typename Arg1, typename Arg2>
struct isProperInstruction<And<Arg1, Arg2>> : public std::true_type {};

template <typename Arg1, typename Arg2>
struct isProperInstruction<Or<Arg1, Arg2>> : public std::true_type {};

template <typename Arg>
struct isProperInstruction<Not<Arg>> : public std::true_type {};

template <typename Arg1, typename Arg2>
struct isProperInstruction<Cmp<Arg1, Arg2>> : public std::true_type {};

/* Program to ciąg instrukcji */

template <typename... T>
struct Program {
  using Instructions = std::tuple<T...>;

  template <typename, typename>
  struct TupleCat;
  template <typename... First, typename... Second>
  struct TupleCat<std::tuple<First...>, std::tuple<Second...>> {
    using type = std::tuple<First..., Second...>;
  };

  template <uint64_t key, size_t value>
  struct KeyValuePair {};
};

template <typename T>
struct IsProgram : public std::false_type {};

template <typename... T>
struct IsProgram<Program<T...>> : public std::true_type {};

// Tu wyszukuję tylko polecenia D, zeby zaktualizować memory, pozostałe powinny
// nie modyfikować memory.
template <size_t memorySize, typename T, typename... Instructions>
struct InitialInstructionsParsing {
  constexpr static void evaluate(State<memorySize, T> &) {}
};

template <size_t memorySize, typename T, uint64_t key, typename value,
          typename... Instructions>
struct InitialInstructionsParsing<memorySize, T,
                                  std::tuple<D<key, value>, Instructions...>> {
  constexpr static void evaluate(State<memorySize, T> &s) {
    if(s.decCount == memorySize) {
        throw "Too many declarations";
    }
    s.declarationIDs[s.decCount] = key;
    s.memoryBlocks[s.decCount] = value::value;
    s.decCount++;
    InitialInstructionsParsing<memorySize, T,
                               std::tuple<Instructions...>>::evaluate(s);
  }
};

template <size_t memorySize, typename T, typename SingleInstruction,
          typename... Instructions>
struct InitialInstructionsParsing<
    memorySize, T, std::tuple<SingleInstruction, Instructions...>> {
  constexpr static void evaluate(State<memorySize, T> &s) {
    static_assert(isProperInstruction<SingleInstruction>::value,
                  "This is not a valid instruction.");
    InitialInstructionsParsing<memorySize, T,
                               std::tuple<Instructions...>>::evaluate(s);
  }
};

/* Parsowanie instrukcji */

template <size_t memorySize, typename T, typename JumpLabel, bool passedLabel,
          typename InstructionsOrigin, typename... Instructions>
struct InstructionsRunner {
  constexpr static void evaluate(State<memorySize, T> &) {}
};

template <size_t memorySize, typename T, uint64_t keyLabel, uint64_t keyLabel2,
          typename InstructionsOrigin, typename... Instructions>
struct InstructionsRunner<memorySize, T, Label<keyLabel>, false,
                          InstructionsOrigin,
                          std::tuple<Label<keyLabel2>, Instructions...>> {
  constexpr static void evaluate(State<memorySize, T> &s) {
    InstructionsRunner<memorySize, T, Label<keyLabel>, keyLabel == keyLabel2,
                       InstructionsOrigin,
                       std::tuple<Instructions...>>::evaluate(s);
  }
};

template <size_t memorySize, typename T, typename keyLabel,
          typename InstructionsOrigin, typename SingleInstruction,
          typename... Instructions>
struct InstructionsRunner<memorySize, T, keyLabel, false, InstructionsOrigin,
                          std::tuple<SingleInstruction, Instructions...>> {
  constexpr static void evaluate(State<memorySize, T> &s) {
    InstructionsRunner<memorySize, T, keyLabel, false, InstructionsOrigin,
                       std::tuple<Instructions...>>::evaluate(s);
  }
};

template <size_t memorySize, typename T, typename keyLabel,
        typename InstructionsOrigin, typename... Instructions>
struct InstructionsRunner<memorySize, T, keyLabel, false, InstructionsOrigin,
        std::tuple<Instructions...>> {
    constexpr static void evaluate(State<memorySize, T> &s) {
        throw "Non-existent label";
    }
};

template <size_t memorySize, typename T, typename keyLabel,
          typename InstructionsOrigin, typename SingleInstruction,
          typename... Instructions>
struct InstructionsRunner<memorySize, T, keyLabel, true, InstructionsOrigin,
                          std::tuple<SingleInstruction, Instructions...>> {
  constexpr static void evaluate(State<memorySize, T> &s) {
    InstructionsRunner<memorySize, T, keyLabel, true, InstructionsOrigin,
                       std::tuple<Instructions...>>::evaluate(s);
  }
};

// Dla polecenia Jmp program rusza od początku tylko z tą nową labelką i flagą
// passedLabel na false.

// Jmp
template <size_t memorySize, typename T, typename keyLabel, uint64_t newLabel,
          typename InstructionsOrigin, typename... Instructions>
struct InstructionsRunner<memorySize, T, keyLabel, true, InstructionsOrigin,
                          std::tuple<Jmp<newLabel>, Instructions...>> {
  constexpr static void evaluate(State<memorySize, T> &s) {
    InstructionsRunner<memorySize, T, Label<newLabel>, false,
                       InstructionsOrigin, InstructionsOrigin>::evaluate(s);
  }
};

// Js
template <size_t memorySize, typename T, typename keyLabel, uint64_t newLabel,
          typename InstructionsOrigin, typename... Instructions>
struct InstructionsRunner<memorySize, T, keyLabel, true, InstructionsOrigin,
                          std::tuple<Js<newLabel>, Instructions...>> {
  constexpr static void evaluate(State<memorySize, T> &s) {
    if (s.sf == 1) {
      InstructionsRunner<memorySize, T, Label<newLabel>, false,
                         InstructionsOrigin, InstructionsOrigin>::evaluate(s);
    } else {
      InstructionsRunner<memorySize, T, keyLabel, true, InstructionsOrigin,
                         std::tuple<Instructions...>>::evaluate(s);
    }
  }
};

// Jz
template <size_t memorySize, typename T, typename keyLabel, uint64_t newLabel,
          typename InstructionsOrigin, typename... Instructions>
struct InstructionsRunner<memorySize, T, keyLabel, true, InstructionsOrigin,
                          std::tuple<Jz<newLabel>, Instructions...>> {
  constexpr static void evaluate(State<memorySize, T> &s) {
    if (s.zf == 1) {
      InstructionsRunner<memorySize, T, Label<newLabel>, false,
                         InstructionsOrigin, InstructionsOrigin>::evaluate(s);
    } else {
      InstructionsRunner<memorySize, T, keyLabel, true, InstructionsOrigin,
                         std::tuple<Instructions...>>::evaluate(s);
    }
  }
};

// Mov
template <size_t memorySize, typename T, typename keyLabel, typename Dst,
          typename Src, typename InstructionsOrigin, typename... Instructions>
struct InstructionsRunner<memorySize, T, keyLabel, true, InstructionsOrigin,
                          std::tuple<Mov<Dst, Src>, Instructions...>> {
  constexpr static void evaluate(State<memorySize, T> &s) {
    Dst::template getLvalue<T, memorySize>(s) =
        Src::template getRvalue<T, memorySize>(s);
    InstructionsRunner<memorySize, T, keyLabel, true, InstructionsOrigin,
                       std::tuple<Instructions...>>::evaluate(s);
  }
};

/* Operacje arytmetyczne */

// Add
template <size_t memorySize, typename T, typename keyLabel, typename Arg1,
          typename Arg2, typename InstructionsOrigin, typename... Instructions>
struct InstructionsRunner<memorySize, T, keyLabel, true, InstructionsOrigin,
                          std::tuple<Add<Arg1, Arg2>, Instructions...>> {
  constexpr static void evaluate(State<memorySize, T> &s) {
    Arg1::template getLvalue<T, memorySize>(s) +=
        Arg2::template getRvalue<T, memorySize>(s);
    s.zf = Arg1::template getRvalue<T, memorySize>(s) == 0;
    s.sf = Arg1::template getRvalue<T, memorySize>(s) < 0;
    InstructionsRunner<memorySize, T, keyLabel, true, InstructionsOrigin,
                       std::tuple<Instructions...>>::evaluate(s);
  }
};

// Sub
template <size_t memorySize, typename T, typename keyLabel, typename Arg1,
          typename Arg2, typename InstructionsOrigin, typename... Instructions>
struct InstructionsRunner<memorySize, T, keyLabel, true, InstructionsOrigin,
                          std::tuple<Sub<Arg1, Arg2>, Instructions...>> {
  constexpr static void evaluate(State<memorySize, T> &s) {
    Arg1::template getLvalue<T, memorySize>(s) -=
        Arg2::template getRvalue<T, memorySize>(s);
    s.zf = Arg1::template getRvalue<T, memorySize>(s) == 0;
    s.sf = Arg1::template getRvalue<T, memorySize>(s) < 0;
    InstructionsRunner<memorySize, T, keyLabel, true, InstructionsOrigin,
                       std::tuple<Instructions...>>::evaluate(s);
  }
};

// Inc
template <size_t memorySize, typename T, typename keyLabel, typename Arg,
          typename InstructionsOrigin, typename... Instructions>
struct InstructionsRunner<memorySize, T, keyLabel, true, InstructionsOrigin,
                          std::tuple<Inc<Arg>, Instructions...>> {
  constexpr static void evaluate(State<memorySize, T> &s) {
    Arg::template getLvalue<T, memorySize>(s)++;
    s.zf = Arg::template getRvalue<T, memorySize>(s) == 0;
    s.sf = Arg::template getRvalue<T, memorySize>(s) < 0;
    InstructionsRunner<memorySize, T, keyLabel, true, InstructionsOrigin,
                       std::tuple<Instructions...>>::evaluate(s);
  }
};

// Dec
template <size_t memorySize, typename T, typename keyLabel, typename Arg,
          typename InstructionsOrigin, typename... Instructions>
struct InstructionsRunner<memorySize, T, keyLabel, true, InstructionsOrigin,
                          std::tuple<Dec<Arg>, Instructions...>> {
  constexpr static void evaluate(State<memorySize, T> &s) {
    Arg::template getLvalue<T, memorySize>(s)--;
    s.zf = Arg::template getRvalue<T, memorySize>(s) == 0;
    s.sf = Arg::template getRvalue<T, memorySize>(s) < 0;
    InstructionsRunner<memorySize, T, keyLabel, true, InstructionsOrigin,
                       std::tuple<Instructions...>>::evaluate(s);
  }
};

/* Operacje logiczne */

// And
template <size_t memorySize, typename T, typename keyLabel, typename Arg1,
          typename Arg2, typename InstructionsOrigin, typename... Instructions>
struct InstructionsRunner<memorySize, T, keyLabel, true, InstructionsOrigin,
                          std::tuple<And<Arg1, Arg2>, Instructions...>> {
  constexpr static void evaluate(State<memorySize, T> &s) {
    Arg1::template getLvalue<T, memorySize>(s) =
        (Arg1::template getRvalue<T, memorySize>(s) &
         Arg2::template getRvalue<T, memorySize>(s));
    s.zf = Arg1::template getRvalue<T, memorySize>(s) == 0;
    InstructionsRunner<memorySize, T, keyLabel, true, InstructionsOrigin,
                       std::tuple<Instructions...>>::evaluate(s);
  }
};

// Or
template <size_t memorySize, typename T, typename keyLabel, typename Arg1,
          typename Arg2, typename InstructionsOrigin, typename... Instructions>
struct InstructionsRunner<memorySize, T, keyLabel, true, InstructionsOrigin,
                          std::tuple<Or<Arg1, Arg2>, Instructions...>> {
  constexpr static void evaluate(State<memorySize, T> &s) {
    Arg1::template getLvalue<T, memorySize>(s) =
        (Arg1::template getRvalue<T, memorySize>(s) |
         Arg2::template getRvalue<T, memorySize>(s));
    s.zf = Arg1::template getRvalue<T, memorySize>(s) == 0;
    InstructionsRunner<memorySize, T, keyLabel, true, InstructionsOrigin,
                       std::tuple<Instructions...>>::evaluate(s);
  }
};

// Not
template <size_t memorySize, typename T, typename keyLabel, typename Arg,
          typename InstructionsOrigin, typename... Instructions>
struct InstructionsRunner<memorySize, T, keyLabel, true, InstructionsOrigin,
                          std::tuple<Not<Arg>, Instructions...>> {
  constexpr static void evaluate(State<memorySize, T> &s) {
    Arg::template getLvalue<T, memorySize>(s) =
        ~(Arg::template getRvalue<T, memorySize>(s));
    s.zf = Arg::template getRvalue<T, memorySize>(s) == 0;
    InstructionsRunner<memorySize, T, keyLabel, true, InstructionsOrigin,
                       std::tuple<Instructions...>>::evaluate(s);
  }
};

/* Operacja porownania */

template <size_t memorySize, typename T, typename keyLabel, typename Arg1,
          typename Arg2, typename InstructionsOrigin, typename... Instructions>
struct InstructionsRunner<memorySize, T, keyLabel, true, InstructionsOrigin,
                          std::tuple<Cmp<Arg1, Arg2>, Instructions...>> {
  constexpr static void evaluate(State<memorySize, T> &s) {
    s.zf = (Arg1::template getRvalue<T, memorySize>(s) ==
            Arg2::template getRvalue<T, memorySize>(s));
    s.sf = (Arg1::template getRvalue<T, memorySize>(s) <
            Arg2::template getRvalue<T, memorySize>(s));
    InstructionsRunner<memorySize, T, keyLabel, true, InstructionsOrigin,
                       std::tuple<Instructions...>>::evaluate(s);
  }
};

template <std::size_t memorySize, typename T>
struct Computer {
 public:
  static_assert(std::is_integral<T>(), "Not an integral type.");

  template <typename ProgramIns>
  static constexpr std::array<T, memorySize> boot() {
    static_assert(IsProgram<ProgramIns>(), "Not a valid program type.");
    State<memorySize, T> computerMemory;

    // Deklaracja zmiennych -- zgodnie z poleceniem, mają być inicjalizowane
    // oddzielnie.
    InitialInstructionsParsing<
        memorySize, T,
        typename ProgramIns::Instructions>::evaluate(computerMemory);

    // Rozpatrzenie pozostałych poleceń.
    InstructionsRunner<
        memorySize, T, Label<0>, true, typename ProgramIns::Instructions,
        typename ProgramIns::Instructions>::evaluate(computerMemory);

    return computerMemory.memoryBlocks;
  }
};

#endif  // ASSEMBLER_COMPUTER_H
