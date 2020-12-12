#include "computer.h"
#include <array>
#include <iostream>

int main() {
    using test_machine = Computer<4, int>;

    // Test for passing non instruction

    using fail_mem = Program<Mem<Num<0>>>;
    using fail_num = Program<Num<0>>;
    using fail_lea = Program<Lea<Id("a")>>;
    using fail_id = Program<Id("a")>;

    test_machine::boot<fail_mem>();
    test_machine::boot<fail_num>();
    test_machine::boot<fail_lea>();
    test_machine::boot<fail_id>();

    // LValue tests

    using fail_l_mov = Program<Mov<Num<0>, Num<0>>>;
    using fail_l_add = Program<Add<Num<0>, Num<0>>>;
    using fail_l_sub = Program<Sub<Num<0>, Num<0>>>;
    using fail_l_inc = Program<Inc<Num<0>>>;
    using fail_l_dec = Program<Dec<Num<0>>>;
    using fail_l_and = Program<And<Num<0>, Num<0>>>;
    using fail_l_or =  Program<Or<Num<0>, Num<0>>>;
    using fail_l_not = Program<Not<Num<0>>>;

    test_machine::boot<fail_l_mov>();
    test_machine::boot<fail_l_add>();
    test_machine::boot<fail_l_sub>();
    test_machine::boot<fail_l_inc>();
    test_machine::boot<fail_l_dec>();
    test_machine::boot<fail_l_and>();
    test_machine::boot<fail_l_or>();
    test_machine::boot<fail_l_not>();

    using fail_not_pvalue = Program<Add<Add<Mem<Num<0>>, Num<69>>, Num<42>>>;
    test_machine::boot<fail_not_pvalue>();

    using fail_undeclared_lea = Program<Mov<Lea<Id("a")>, Num<42>>>;
    test_machine::boot<fail_undeclared_lea>();

};
