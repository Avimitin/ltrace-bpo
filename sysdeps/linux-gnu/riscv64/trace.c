/*
 * This file is part of ltrace.
 * Copyright (C) 2022 Kai Zhang (laokz)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <stdio.h>
#include <sys/wait.h>
#include "backend.h"
#include "proc.h"
#include "ptrace.h"

extern long
riscv64_read_gregs(struct Process *proc, struct user_regs_struct *regs);

void
get_arch_dep(struct Process *proc)
{
}

int
syscall_p(struct Process *proc, int status, int *sysnum)
{
    if (WIFSTOPPED(status)
            && WSTOPSIG(status) == (SIGTRAP | proc->tracesysgood)) {

        struct user_regs_struct regs;
        if (riscv64_read_gregs(proc, &regs) == -1)
            return -1;

        /* ecall has no compressed format */
        switch (ptrace(PTRACE_PEEKTEXT, proc->pid, regs.pc - 4, 0) &
                    0xFFFFFFFF) {
            case 0x73:
                break;
            case -1:
                perror("PTRACE_PEEKTEXT");
                return -1;
            default:
                return 0;
        }

        *sysnum = regs.a7;
        size_t i = proc->callstack_depth - 1;
        if (proc->callstack_depth > 0
                && proc->callstack[i].is_syscall
                && proc->callstack[i].c_un.syscall == (int)regs.a7) {
            return 2;
        }
        return 1;
    }
    return 0;
}

int
arch_atomic_singlestep(struct Process *proc, struct breakpoint *sbp,
		       int (*add_cb)(void *addr, void *data),
		       void *add_cb_data) {
    if (!ptrace(PTRACE_SYSCALL, proc->pid, 0, 0)) return 0;
    return 1;
}
