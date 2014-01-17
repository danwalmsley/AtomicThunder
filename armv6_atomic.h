#ifndef ARMV6ATOMIC_H
#define ARMV6ATOMIC_H

static inline bool compare_and_swap32(UInt32* mem, UInt32 oldval, UInt32 newval)
{
    register int result;

    asm volatile (
     "ldrex    r0, [%1]         \n\t"   /*exclusive load of ptr */
     "cmp      r0,  %2          \n\t"   /*compare the oldval ==  *ptr */
     #if defined(__thumb__)
     "ite eq                    \n\t"
     #endif
     "strexeq  %0,  %3, [%1]\n\t" /*store if eq, strex+eq*/
     #if defined(__thumb__)
     "clrexne                   \n\t"
     #endif
     : "=&r" (result)
     : "r"(mem), "r"(oldval),"r"(newval)
     : "r0"
     );

    return result == 0;
}

static inline bool compare_and_swap_ptr(void** mem, void* oldval, void* newval)
{
    return compare_and_swap32((UInt32*)mem, (UInt32)oldval, (UInt32)newval);
}

#endif
