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
#ifndef _SYNC_H
#define _SYNC_H
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
/*Mutex*/
struct mutex;
typedef struct mutex *mutex_t;

mutex_t mutex_create();
void mutex_destroy(mutex_t *m);
inline int32_t mutex_lock(mutex_t m);
inline int32_t mutex_try_lock(mutex_t m);
inline int32_t mutex_unlock(mutex_t m);

/*Condition*/
struct condition;
typedef struct condition *condition_t;

condition_t condition_create();
void condition_destroy(condition_t *c);
inline int32_t condition_wait(condition_t c,mutex_t m);
int32_t condition_timedwait(condition_t c,mutex_t m,int32_t ms);
inline int32_t condition_signal(condition_t c);
inline int32_t condition_broadcast(condition_t c);

/*Barrior*/
struct barrior;
typedef struct barrior *barrior_t;

barrior_t barrior_create(int32_t);
void barrior_destroy(barrior_t*);
void barrior_wait(barrior_t);

#endif
