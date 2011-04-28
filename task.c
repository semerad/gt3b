/*
    task - cooperative multitasking
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


#include "task.h"


// main OPER task
TCB OPER;


// pointer to current task
TCB *ptid;


// initialise tasks
void task_init(void) {
    // initialize current task id
    ptid = &OPER;
    // OPER is running -> running task has ASLEEP state
    sleep(OPER);
    // initialize LINK in OPER task
    OPER.link = &OPER;
}


// activate task - set pointers and AWAKE
void _do_activate(TCB *task, u8 *stack, u16 stack_size,
		  void (*function)(void)) {
    u8 *stack_ret = stack + stack_size - 2;
    *(u16 *)stack_ret = (u16)function;
    task->hwstack = stack_ret - 1;  // SP points below last value
    task->link = 0;
    task->status = _AWAKE;
}


// build - link in task
void _do_build(TCB *task) {
    task->status = _ASLEEP;	// do not run this yet
    task->link = &OPER;
    OPER.link = task;
}




void stop(void) {
}

