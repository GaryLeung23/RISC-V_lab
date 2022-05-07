#include <uapi/syscall.h>
#include "syscall.h"

/*
 * clone->__clone->ecall ->do_exception_vector->do_exception
 * ->riscv_svc_handler->riscv_syscall_common->invoke_syscall->syscall_table[nr]()->__riscv_sys_clone
 * ->do_fork
 */
/*
 * 总的来说,这里是发生异常,然后分配pid,内核栈,并将父进程的中断现场复制到子进程,但是修改其中的sp为已分配的
 * 用户栈,并将子进程放入调度器中,当调度到子进程就达到ret_from_fork,后续从异常返回后就从ecall的下一条指令开始运行。
 * 
 */
int clone(int (*fn)(void *arg), void *child_stack,
		int flags, void *arg)
{
	//child_stack为栈底,flag=0
	return __clone(fn, child_stack, flags, arg);
}

/*malloc -> syscall -> ecall ->do_exception_vector->do_exception
 * ->riscv_svc_handler->riscv_syscall_common->invoke_syscall->syscall_table[nr]()->__riscv_sys_malloc
 */
unsigned long malloc(void)
{
	return syscall(__NR_malloc);
}
