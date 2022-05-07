#include <uapi/syscall.h>
#include "syscall.h"

/*malloc -> syscall -> ecall ->do_exception_vector->do_exception
 * ->riscv_svc_handler->riscv_syscall_common->invoke_syscall->syscall_table[nr]()->__riscv_sys_malloc
 */
unsigned long malloc(void)
{
	return syscall(__NR_malloc);
}
