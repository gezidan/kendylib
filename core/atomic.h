/*	
    Copyright (C) <2012>  <huangweilook@21cn.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/	
#ifndef _ATOMIC_H
#define _ATOMIC_H
#include <stdint.h>

typedef volatile int8_t  atomic_8_t;
typedef volatile int16_t atomic_16_t;
typedef volatile int32_t atomic_32_t;
typedef volatile int64_t atomic_64_t;

#define COMPARE_AND_SWAP(PTR,OLD,NEW) __sync_val_compare_and_swap(PTR,OLD,NEW)  
#define ATOMIC_INCREASE(PTR) __sync_add_and_fetch(PTR,1) //ԭ�Ӽ�1,���ز�������ֵ
#define ATOMIC_DECREASE(PTR) __sync_sub_and_fetch(PTR,1) //ԭ�Ӽ�1,���ز�������ֵ
#define ATOMIC_SET(PTR,VAL)  __sync_lock_test_and_set(PTR,VAL)
#endif
