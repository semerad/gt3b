/*
    task - include file
    Copyright (C) 2011 Pavel Semerad

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


#ifndef _TASK_INCLUDED
#define _TASK_INCLUDED


#include "stm8.h"


// Task Control Block
struct TCB_s {
    u8 status;
    struct TCB_s *link;
    u8 *hwstack;
};
typedef struct TCB_s TCB;
extern TCB *ptid;


// TASK - create TCB, stack
#define TASK(name, stack_size) \
    TCB name; \
    @near u8 name ## _stack[stack_size]; \
    const u16 name ## _stack_size = stack_size
#define E_TASK(name) \
    extern TCB name


// sleep, awake
#define _ASLEEP 0
#define _AWAKE  0xff
#define sleep(task)  task.status = _ASLEEP
#define awake(task)  task.status = _AWAKE


// activate
#define activate(task, function) \
    _do_activate(&task, task ## _stack, task ## _stack_size, function);
extern void _do_activate(TCB *task, u8 *stack, u16 stack_size,
			 void (*function)(void));
    

// build
#define build(task) \
    _do_build(&task)
extern void _do_build(TCB *task);


// pause, stop
extern void pause(void);
extern void stop(void);


// OPER task
E_TASK(OPER);


#endif

