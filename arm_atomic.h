/*
 * arm_atomic.h
 *
 *  Created on: 9 jan. 2014
 *      Author: d_walmsley
 */

#ifndef ARM_ATOMIC_H_
#define ARM_ATOMIC_H_

static inline int simple_atomic_swap(int* mem, int newval)
{
  int oldval;
  asm volatile ("swp    %0, %1, [%2]"
               :"=&r"(oldval)
               :"r"  (newval),
                "r"  (&mem)
               :"memory");
  return oldval;
}

static inline void* simple_atomic_swap_ptr(void** mem, void* newval)
{
    void* oldval;
    asm volatile ("swp    %0, %1, [%2]"
               :"=&r"(oldval)
               :"r"  (newval),
                "r"  (mem)              //No &mem here because pointer to pointer is like a reference.
               :"memory");
    return oldval;
}

// The other atomic operations are emulated by means of a sentinel value, -1:
// the variable is first swapped with the sentinel, the operation is performed and
// the modified value is written back.  Any thread that wants to read the
// variable must re-try if it reads the sentinel.

static int sentinel = -1;

static inline void* atomic_read_and_lock_ptr(void** mem)
{
    do
    {
        void* val = simple_atomic_swap_ptr(mem, &sentinel);

        if(val != &sentinel)
        {
            return val;
        }

    }while(1);

    return 0;
}

static inline int atomic_read_and_lock(int* mem)
{
    do
    {
        int val = simple_atomic_swap(mem,-1);

        if (val != -1)
        {
            return val;
        }

    } while (1);

    return 0;
}

static inline int atomic_post_inc(int* mem)
{
  int val = atomic_read_and_lock(mem);
  *mem = val + 1;
  return val;
}

static inline int atomic_post_dec(int* mem)
{
  int val = atomic_read_and_lock(mem);
  *mem = val - 1;
  return val;
}

static inline int atomic_swap(int* mem, int newval)
{
  int oldval = atomic_read_and_lock(mem);
  *mem = newval;
  return oldval;
}

static inline void* atomic_compare_and_swap_ptr(void** mem, void* expect, void* newval)
{
   void* oldval = atomic_read_and_lock_ptr(mem);

   if(oldval == expect)
   {
       *mem = newval;
   }
   else
   {
       *mem = oldval;
   }

   return oldval;
}

static inline int atomic_compare_and_swap(int* mem, int expect, int newval)
{
  int oldval = atomic_read_and_lock(mem);

  if (oldval==expect)
  {
    *mem = newval;
  }
  else
  {
    *mem = oldval;
  }
  return oldval;
}

static inline int atomic_read(volatile int* mem)
{
  do {
    int val = *mem;
    if (val != -1) {
      return val;
    }
  } while (1);

  return 0;
}

static inline int atomic_write(int* mem, int newval)
{
  atomic_read_and_lock(mem);
  *mem = newval;
  return newval;
}

#endif /* ARM_ATOMIC_H_ */
