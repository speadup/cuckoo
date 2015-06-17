#ifndef _ATOMIC_H_
#define _ATOMIC_H_

#ifdef __x86_64__

//没有处理 64位的 x86
#elif __i386__

#include "arch/x86/atomic32_x86.h"

#elif __MIPSEL__

#include "arch/mips/atomic32_mips.h"

#else

//该处只处理了 arm 32
#include "arch/arm/atomic32_arm.h"

#endif

#endif  //_ATOMIC_H_

