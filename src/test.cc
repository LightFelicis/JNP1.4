#include "computer.h"
#include <cassert>
#include <iostream>

int main() {
    if (Program<>::MatchKeyMapping<
        Id("d"), 
        typename Program<>::Mapping<
                D<Id("a"), Num<10>>, 
                D<Id("b"), Num<20>>, 
                Lea<Id("c")>, 
                D<Id("d"), Num<30>>, 
                D<Id("e"), Num<40>>
                >::type>::value 
        == 2) {
            std::cout << "PRZESZŁO :)\n";
        }
}