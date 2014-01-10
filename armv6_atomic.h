#ifndef ARMV6ATOMIC_H
#define ARMV6ATOMIC_H
//
// ARMV6 , ARMV7 Atomic primitives
// Based on knowlege
// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0489b/Cihbghef.html
//
// Autor Artur Bac 2010 EBASoft
//
// Under BSD Licence
//
// version 1.0.0
// Written for Bada & GCC C++ but will also work on any other platform with Cortex CPU like IPhone, Android
// ARMv6 introduces a new mechanism, known as "exclusives",
// using the LDREX and STREX instructions.
// Direct use of these instructions is not
// recommended for new code (see "GCC atomic primitives" for a better alternative).
// However, to assist with understanding existing code, a quick overview follows:
//      LDREX   r0, [r1]
//            // do something with r0
//           // no other load or store operations can occur between a LDREX and its corresponding STREX
//       STREX   r2, r0, [r1]
//            // now r2 = 0 if the new value was stored
//            // r2 = 1 if the store was abandoned
//
//Because the STREX is allowed to "fail", it is no longer necessary to lock out other bus activity,
// or stop other threads from executing (whether on the same CPU or on another CPU in a multiprocessor system);
// atomic use in a thread can preempt a pending atomic in another thread.
// For this reason, these primitives are usually also placed in a loop.
//Note that extra load and store instruction (of any kind) between LDREX and STREX can cause the STREX always to fail in some implementations,
//which is why you shouldn't access memory between the LDREX and the corresponding STREX,
//except in cases where it's sensible to abandon the operation such as context switches or exceptions.

#ifndef DISABLE_MESSAGES
#pragma message "Using ARMv6 atomic primitives"
#endif
typedef long long a_int64_t;

//It ensures that all explicit memory accesses that appear in program order
//before the DMB instruction are observed before any explicit
//memory accesses that appear in program order after the DMB instruction.
//DMB does not affect the ordering of instructions that do not access memory.
inline void sync_synchronize() { asm volatile( "dmb;"); }
inline void cpu_yield() { asm volatile ("yield;"); }
//These builtins perform an atomic compare and swap.
//That is, if the current value of *ptr is oldval, then write newval into *ptr.
//The “val” version returns the contents of *ptr before the operation.
namespace atomic_internal__
   {
    typedef unsigned int size_t;
    template<typename T, typename U, size_t sz>
    struct sync_bool_compare_and_swap_internal
	{
	};
    template<typename T, typename U>
    struct sync_bool_compare_and_swap_internal<T,U,sizeof(long long)>
	 {
   	 bool operator()( T *ptr, U oldval, U newval ) const
	     {
	     register int result;
	     asm volatile (
		     "ldrexd   r2, [%1]         \n\t" /*exclusive load of ptr*/
		     "cmp      r2,  %2          \n\t"/*compare the low reg oldval == low *ptr*/
    #if defined(__thumb__)
		     "itte eq\n\t"
    #endif
		     "cmpeq    r3, %H2          \n\t"/*compare the hireg oldval == hireg *ptr*/
		     "strexdeq %0,  %3, [%1]\n\t" /*store if eq, strex+eq*/
    #if defined(__thumb__)
		     "clrexne			\n\t"
    #endif
		     : "=&r" (result)
		     : "r"(ptr), "r"(oldval),"r"(newval)
		     : "r2", "r3"
		     );
	     return result ==0;
	     }
	};
    template<typename T, typename U>
    struct sync_bool_compare_and_swap_internal<T,U,sizeof(long)>
		{
		bool operator()( T *ptr, U oldval, U newval ) const
	   	{
			 register int result;
			 asm volatile (
				 "ldrex    r0, [%1]         \n\t" 	/*exclusive load of ptr */
				 "cmp      r0,  %2          \n\t"	/*compare the oldval ==  *ptr */
	#if defined(__thumb__)
				 "ite eq\n\t"
	#endif
				 "strexeq  %0,  %3, [%1]\n\t" /*store if eq, strex+eq*/
	#if defined(__thumb__)
				 "clrexne"
	#endif
				 : "=&r" (result)
				 : "r"(ptr), "r"(oldval),"r"(newval)
				 : "r0"
				 );
			return result ==0;
			}
	};
    template<typename T, typename U>
    struct sync_bool_compare_and_swap_internal<T,U,sizeof(short)>
		{
		bool operator()( T *ptr, U oldval, U newval ) const
			 {
			 register int result;
			 asm volatile (
			 "ldrexh   r1, [%1]         \n\t" /*exclusive load of ptr*/
				 "cmp      r1,  %2          \n\t"/*compare the low reg oldval == low *ptr*/
	#if defined(__thumb__)
				 "ite eq\n\t"
	#endif
				 "strexheq %0,  %3, [%1]\n\t" /*store if eq, strex+eq*/
	#if defined(__thumb__)
				 "clrexne"
	#endif
				 : "=&r" (result)
				 : "r"(ptr), "r"(oldval),"r"(newval)
				 : "r1"
				 );
			 return result ==0;
			 }
		};
    template<typename T, typename U>
    struct sync_bool_compare_and_swap_internal<T,U,sizeof(char)>
		{
		bool operator()( T *ptr, U oldval, U newval ) const
			 {
			 register int result;
			 asm volatile (
				 "ldrexb   r1, [%1]         \n\t" /*exclusive load of ptr*/
				 "cmp      r1,  %2          \n\t"/*compare the low reg oldval == low *ptr*/
	#if defined(__thumb__)
				 "ite eq\n\t"
	#endif
				 "strexbeq %0,  %3, [%1]" /*store if eq, strex+eq*/
	#if defined(__thumb__)
				 "clrexne"
	#endif
				 : "=&r" (result)
				 : "r"(ptr), "r"(oldval),"r"(newval)
				 : "r1"
				);
			 return result ==0;
			 }
		};
}
template<typename T, typename U>
 bool sync_bool_compare_and_swap ( T *ptr, U oldval, U newval )
    {
    return atomic_internal__::sync_bool_compare_and_swap_internal<T,U,sizeof(T)>()(ptr,oldval,newval);
    }

namespace atomic_internal__
   {
    template<typename T, typename U, size_t sz>
    struct sync_lock_test_and_set
	{
	};
    template<typename T, typename U>
    struct sync_lock_test_and_set<T,U,sizeof(long long)>
		 {
   	 T operator()( T *ptr, U value ) const
			 {
			 register T result;
			 asm (
				 "1: ldrexd   %0,  [%1]		\n\t"
					 "strexd   r4,   %2, [%1]		\n\t"
					 "cmp     r4,   #0		\n\t"
					 "bne     1b			\n\t "
					  : "=&r" (result)
				: "r"(ptr), "r"(value)
				: "r4"
				 );
			 return result;
			 }
	 };
    template<typename T, typename U>
    struct sync_lock_test_and_set<T,U,sizeof(long)>
		{
		T operator()( T *ptr, U value ) const
			 {
			 register T result;
			 asm (
				 "1: ldrex   %0,  [%1]		\n\t"
					 "strex   r4,   %2, [%1]		\n\t"
					 "cmp     r4,   #0		\n\t"
					 "bne     1b			\n\t "
					  : "=&r" (result)
				: "r"(ptr), "r"(value)
				: "r4"
				 );
			 return result;
			 }
		};
    template<typename T, typename U>
    struct sync_lock_test_and_set<T,U,sizeof(short)>
		{
		T operator()( T *ptr, U value ) const
			 {
			 register T result;
			 asm (
				 "1: ldrexh   %0,  [%1]		\n\t"
					 "strexh   r4,   %2, [%1]		\n\t"
					 "cmp     r4,   #0		\n\t"
					 "bne     1b			\n\t "
					  : "=&r" (result)
				: "r"(ptr), "r"(value)
				: "r4"
				 );
			 return result;
			 }
		};
    template<typename T, typename U>
    struct sync_lock_test_and_set<T,U,sizeof(char)>
		{
		T operator()( T *ptr, U value ) const
			 {
			 register T result;
			 asm (
				 "1: ldrexb   %0,  [%1]\n\t"
					 "strexb   r4,   %2, [%1]\n\t"
					 "cmp     r4,   #0\n\t"
					 "bne     1b	\n\t "
					  : "=&r" (result)
				: "r"(ptr), "r"(value)
				: "r4"
				 );
			 return result;
			 }
		};
    }
template<typename T, typename U>
inline T sync_lock_test_and_set ( T *ptr, U value )
    {
    return atomic_internal__::sync_lock_test_and_set<T,U,sizeof(T)>()(ptr,value);
    }

namespace atomic_internal__
   {
    template<typename T, typename U, size_t sz>
    struct sync_fetch_and_add
	{
	};
    template<typename T, typename U>
    struct sync_fetch_and_add<T,U,sizeof(long long)>
		{
		T operator()( T *ptr, U value ) const
			 {
			 register T result;
			 asm volatile (
				  "1: ldrexd   %0,  [%1]		\n\t"
				"adds     r2,   %0,  %2		\n\t"
				"adc      r3,  %H0,  %H2	\n\t"
				"strexd   r1,   r2,  [%1]	\n\t"
				"cmp      r1,   #0		\n\t"
				"bne      1b"
				: "=&r" (result)
				: "r"(ptr), "r"(value)
				: "r1","r2","r3"
				 );
			 return result;
			 }
		};
    template<typename T, typename U>
    struct sync_fetch_and_add<T,U,sizeof(long)>
		{
		T operator()( T *ptr, U value ) const
			 {
			 register T result;
			 asm volatile (
				  "1: ldrex   %0,  [%1]	\n\t"
				"add     r1,   %0,  %2	\n\t"
				"strex   r2,   r1, [%1]	\n\t"
				"cmp     r2,   #0	\n\t"
				"bne     1b"
				: "=&r" (result)
				: "r"(ptr), "r"(value)
				: "r1","r2"
				 );
			 return result;
			 }
		};
    template<typename T, typename U>
    struct sync_fetch_and_add<T,U,sizeof(short)>
		{
		T operator()( T *ptr, U value ) const
			 {
			 register T result;
			 asm volatile (
				  "1: ldrexh  %0,  [%1]	\n\t"
				"add     r1,   %0,  %2	\n\t"
				"strexh  r2,   r1, [%1]	\n\t"
				"cmp     r2,   #0	\n\t"
				"bne     1b"
				: "=&r" (result)
				: "r"(ptr), "r"(value)
				: "r1","r2"
				 );
			 return result;
			 }
		};
    template<typename T, typename U>
    struct sync_fetch_and_add<T,U,sizeof(char)>
		{
		T operator()( T *ptr, U value ) const
			 {
			 register T result;
			 asm volatile (
				  "1: ldrexb  %0,  [%1]	\n\t"
				"add     r1,   %0,  %2	\n\t"
				"strexb  r2,   r1, [%1]	\n\t"
				"cmp     r2,   #0	\n\t"
				"bne     1b"
				: "=&r" (result)
				: "r"(ptr), "r"(value)
				: "r1","r2"
				 );
			 return result;
			 }
		};
   }
template<typename T, typename U>
inline T sync_fetch_and_add ( T *ptr, U value )
    {
    return atomic_internal__::sync_fetch_and_add<T,U,sizeof(T)>()(ptr,value);
    }

namespace atomic_internal__
   {
    template<typename T, typename U, size_t sz>
    struct sync_add_and_fetch
	{
	};
    template<typename T, typename U>
    struct sync_add_and_fetch<T,U,sizeof(long long)>
		{
		T operator()( T *ptr, U value ) const
			 {
			 register T result;
			 asm volatile (
			  "1:   ldrexd   %0,  [%1]				\n\t"
					 "adds     %0,   %0,  %2		\n\t"
					 "adc      %H0,  %H0,  %H2		\n\t"
					 "strexd   r1,   %0,  [%1]		\n\t"
					 "cmp      r1,   #0				\n\t"
					 "bne      1b"
				: "=&r" (result)
				: "r"(ptr), "r"(value)
				: "r1"
				 );
			 return result;
			 }
		};

    template<typename T, typename U>
    struct sync_add_and_fetch<T,U,sizeof(long)>
		{
		T operator()( T *ptr, U value ) const
			 {
			 register T result;
			 asm volatile (
			  "1:   ldrex   %0,  [%1]			\n\t"
					 "add     %0,   %0,  %2		\n\t"
					 "strex   r1,   %0, [%1]	\n\t"
					 "cmp     r1,   #0			\n\t"
					 "bne     1b"
				: "=&r" (result)
				: "r"(ptr), "r"(value)
				: "r1"
				 );
			 return result;
			 }
		};
    template<typename T, typename U>
    struct sync_add_and_fetch<T,U,sizeof(short)>
		{
		T operator()( T *ptr, U value ) const
			 {
			 register T result;
			 asm volatile (
			  "1:   ldrexh  %0,  [%1]			\n\t"
					 "add     %0,   %0,  %2		\n\t"
					 "strexh  r1,   %0, [%1]	\n\t"
					 "cmp     r1,   #0			\n\t"
					 "bne     1b"
				: "=&r" (result)
				: "r"(ptr), "r"(value)
				: "r1"
				 );
			 return result;
			 }
		};
    template<typename T, typename U>
    struct sync_add_and_fetch<T,U,sizeof(char)>
		{
		T operator()( T *ptr, U value ) const
			 {
			 register T result;
			 asm volatile (
				  "1:   ldrexb  %0,  [%1]			\n\t"
						 "add     %0,   %0,  %2		\n\t"
						 "strexb  r1,   %0, [%1]	\n\t"
						 "cmp     r1,   #0			\n\t"
						 "bne     1b"
				: "=&r" (result)
				: "r"(ptr), "r"(value)
				: "r1"
				 );
			 return result;
			 }
		};
   }
template<typename T, typename U>
inline T sync_add_and_fetch ( T *ptr, U value )
    {
    return atomic_internal__::sync_add_and_fetch<T,U,sizeof(T)>()(ptr,value);
    }

namespace atomic_internal__
   {
    template<typename T, typename U, size_t sz>
    struct sync_fetch_and_sub
	{
	};
    template<typename T, typename U>
    struct sync_fetch_and_sub<T,U,sizeof(long long)>
		{
		T operator()( T *ptr, U value ) const
			 {
			 register T result;
			 asm volatile (
				  "1: ldrexd   %0,  [%1]		\n\t"
				"subs     r2,   %0,  %2		\n\t"
				"sbc      r3,  %H0,  %H2	\n\t"
				"strexd   r1,   r2,  [%1]	\n\t"
				"cmp      r1,   #0		\n\t"
				"bne      1b"
				: "=&r" (result)
				: "r"(ptr), "r"(value)
				: "r1","r2","r3"
				 );
			 return result;
			 }
		};
    template<typename T, typename U>
    struct sync_fetch_and_sub<T,U,sizeof(long)>
		{
		T operator()( T *ptr, U value ) const
			 {
			 register T result;
			 asm volatile (
				  "1: ldrex   %0,  [%1]	\n\t"
				"sub     r1,   %0,  %2	\n\t"
				"strex   r2,   r1, [%1]	\n\t"
				"cmp     r2,   #0	\n\t"
				"bne     1b"
				: "=&r" (result)
				: "r"(ptr), "r"(value)
				: "r1","r2"
				 );
			 return result;
			 }
		};
    template<typename T, typename U>
    struct sync_fetch_and_sub<T,U,sizeof(short)>
		{
		T operator()( T *ptr, U value ) const
			 {
			 register T result;
			 asm volatile (
				  "1: ldrexh  %0,  [%1]	\n\t"
				"sub     r1,   %0,  %2	\n\t"
				"strexh  r2,   r1, [%1]	\n\t"
				"cmp     r2,   #0	\n\t"
				"bne     1b"
				: "=&r" (result)
				: "r"(ptr), "r"(value)
				: "r1","r2"
				 );
			 return result;
			 }
		};
    template<typename T, typename U>
    struct sync_fetch_and_sub<T,U,sizeof(char)>
		{
		T operator()( T *ptr, U value ) const
			 {
			 register T result;
			 asm volatile (
				  "1: ldrexb  %0,  [%1]	\n\t"
				"sub     r1,   %0,  %2	\n\t"
				"strexb  r2,   r1, [%1]	\n\t"
				"cmp     r2,   #0	\n\t"
				"bne     1b"
				: "=&r" (result)
				: "r"(ptr), "r"(value)
				: "r1","r2"
				 );
			 return result;
			 }
		};
   }
template<typename T, typename U>
inline T sync_fetch_and_sub ( T *ptr, U value )
    {
    return atomic_internal__::sync_fetch_and_sub<T,U,sizeof(T)>()(ptr,value);
    }

namespace atomic_internal__
   {
    template<typename T, typename U, size_t sz>
    struct sync_sub_and_fetch
	{
	};
    template<typename T, typename U>
    struct sync_sub_and_fetch<T,U,sizeof(long long)>
		{
		T operator()( T *ptr, U value ) const
			 {
			 register T result;
			 asm volatile (
			  "1:   ldrexd   %0,  [%1]				\n\t"
					 "subs     %0,   %0,  %2		\n\t"
					 "sbc      %H0,  %H0,  %H2		\n\t"
					 "strexd   r1,   %0,  [%1]		\n\t"
					 "cmp      r1,   #0				\n\t"
					 "bne      1b"
				: "=&r" (result)
				: "r"(ptr), "r"(value)
				: "r1"
				 );
			 return result;
			 }
		};

    template<typename T, typename U>
    struct sync_sub_and_fetch<T,U,sizeof(long)>
		{
		T operator()( T *ptr, U value ) const
			 {
			 register T result;
			 asm volatile (
			  "1:   ldrex   %0,  [%1]			\n\t"
					 "sub     %0,   %0,  %2		\n\t"
					 "strex   r1,   %0, [%1]	\n\t"
					 "cmp     r1,   #0			\n\t"
					 "bne     1b"
				: "=&r" (result)
				: "r"(ptr), "r"(value)
				: "r1"
				 );
			 return result;
			 }
		};
    template<typename T, typename U>
    struct sync_sub_and_fetch<T,U,sizeof(short)>
		{
		T operator()( T *ptr, U value ) const
			 {
			 register T result;
			 asm volatile (
			  "1:   ldrexh  %0,  [%1]			\n\t"
					 "sub     %0,   %0,  %2		\n\t"
					 "strexh  r1,   %0, [%1]	\n\t"
					 "cmp     r1,   #0			\n\t"
					 "bne     1b"
				: "=&r" (result)
				: "r"(ptr), "r"(value)
				: "r1"
				 );
			 return result;
			 }
		};
    template<typename T, typename U>
    struct sync_sub_and_fetch<T,U,sizeof(char)>
		{
		T operator()( T *ptr, U value ) const
			 {
			 register T result;
			 asm volatile (
				  "1:   ldrexb  %0,  [%1]			\n\t"
						 "sub     %0,   %0,  %2		\n\t"
						 "strexb  r1,   %0, [%1]	\n\t"
						 "cmp     r1,   #0			\n\t"
						 "bne     1b"
				: "=&r" (result)
				: "r"(ptr), "r"(value)
				: "r1"
				 );
			 return result;
			 }
		};
   }
template<typename T, typename U>
inline T sync_sub_and_fetch ( T *ptr, U value )
    {
    return atomic_internal__::sync_sub_and_fetch<T,U,sizeof(T)>()(ptr,value);
    }


template<typename T>
inline T sync_increment_and_fetch ( T *ptr )
    {
    return atomic_internal__::sync_add_and_fetch<T,int,sizeof(T)>()(ptr,1);
    }

template<typename T>
inline T sync_decrement_and_fetch ( T *ptr )
    {
    return atomic_internal__::sync_sub_and_fetch<T,int,sizeof(T)>()(ptr,1);
    }

template<typename T>
inline T sync_fetch_and_increment ( T *ptr )
    {
    return atomic_internal__::sync_fetch_and_add<T,int,sizeof(T)>()(ptr,1);
    }

template<typename T>
inline T sync_fetch_and_decrement( T *ptr )
    {
    return atomic_internal__::sync_fetch_and_sub<T,int,sizeof(T)>()(ptr,1);
    }

#endif
