#ifndef _ATOMIC32_x86_H_
#define _ATOMIC32_x86_H_

#define LOCK_PREFIX_HERE \
		".section .smp_locks,\"a\"\n"   \
		".balign 4\n"           \
		".long 671f - .\n" /* offset */ \
		".previous\n"           \
		"671:"

#define LOCK_PREFIX LOCK_PREFIX_HERE "\n\tlock; "

typedef struct  {  volatile int counter;  }  atomic_t;

/**
 * atomic_read - read atomic variable
 * @v: pointer of type atomic_t
 *
 * Atomically reads the value of @v.
 * 该函数对原子类型的变量进行原子读操作，它返回原子类型的变量v的值。
 */
static inline int atomic_read(const atomic_t *v)
{
	return (*(volatile int *)&(v)->counter);
}

/**
 * atomic_set - set atomic variable
 * @v: pointer of type atomic_t
 * @i: required value
 *
 * Atomically sets the value of @v to @i.
 * 该函数设置原子类型的变量v的值为i。
 */
static inline void atomic_set(atomic_t *v, int i)
{
	v->counter = i;
}

/**
 * atomic_add - add integer to atomic variable
 * @i: integer value to add
 * @v: pointer of type atomic_t
 *
 * Atomically adds @i to @v.
 * 该函数给原子类型的变量v增加值i。
 */
static inline void atomic_add(int i, atomic_t *v)
{
	asm volatile(LOCK_PREFIX "addl %1,%0"
		     : "+m" (v->counter)
		     : "ir" (i));
}

/**
 * atomic_sub - subtract integer from atomic variable
 * @i: integer value to subtract
 * @v: pointer of type atomic_t
 *
 * Atomically subtracts @i from @v.
 * 该函数从原子类型的变量v中减去i。
 */
static inline void atomic_sub(int i, atomic_t *v)
{
	asm volatile(LOCK_PREFIX "subl %1,%0"
		     : "+m" (v->counter)
		     : "ir" (i));
}


/**
 * atomic_inc - increment atomic variable
 * @v: pointer of type atomic_t
 *
 * Atomically increments @v by 1.
 * 该函数对原子类型变量v原子地增加1。
 */
static inline void atomic_inc(atomic_t *v)
{
	asm volatile(LOCK_PREFIX "incl %0"
		     : "+m" (v->counter));
}

/**
 * atomic_dec - decrement atomic variable
 * @v: pointer of type atomic_t
 *
 * Atomically decrements @v by 1.
 * 该函数对原子类型的变量v原子地减1。
 */
static inline void atomic_dec(atomic_t *v)
{
	asm volatile(LOCK_PREFIX "decl %0"
		     : "+m" (v->counter));
}

#endif  //_ATOMIC32_x86_H_

