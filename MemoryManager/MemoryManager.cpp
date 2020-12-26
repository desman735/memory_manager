#include <iostream>

#include "fixed_size_allocator.h"

int main()
{
    fixed_size_allocator<16, 256> test("test");

    int* allocated_addrs[23]; 
    for(int count = 0; count < 16; ++count)
    {
        allocated_addrs[count] = (int*) test.alloc(sizeof(int));
        *allocated_addrs[count] = count;
    }

#ifndef NDEBUG
    test.dumpStat();
    test.dumpBlocks();
#endif

    for(int count = 16; count < 23; ++count)
    {
        allocated_addrs[count] = (int*) test.alloc(sizeof(int));
        *allocated_addrs[count] = count;
    }

    for(int count = 0; count < 23; ++count)
    {
        std::cout << (void*) allocated_addrs[count]<< ": " << *allocated_addrs[count] << std::endl;
    }

#ifndef NDEBUG
    test.dumpStat();
    test.dumpBlocks();
#endif

    for(int count = 0; count < 23; ++count)
    {
        test.free(allocated_addrs[count]);
    }

#ifndef NDEBUG
    test.dumpStat();
    test.dumpBlocks();
#endif

    return 0;
}
