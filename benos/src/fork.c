#include <sched.h>
#include "string.h"
#include "memset.h"
#include "asm/csr.h"
#include "printk.h"
// #include <irq.h>

/* 把0号进程的内核栈 编译链接到.data.init_task段中 */
#define __init_task_data __attribute__((__section__(".data.init_task")))

/* 0号进程为init进程 */

union task_union init_task_union __init_task_data
			= {INIT_TASK(init_task_union.task),};


/* 定义一个全局的task_struct数组来存放进程的PCB*/
struct task_struct *g_task[NR_TASK] = {&init_task_union.task,};

unsigned long total_forks;

static int find_empty_task(void)
{
	int i;

	for (i = 0 ; i < NR_TASK; i++) {
		if (!g_task[i])
			return i;
	}

	return -1;
}

/*
 * pt_regs存储在栈顶
 */
static struct pt_regs *task_pt_regs(struct task_struct *tsk)
{
	unsigned long p;

	p = (unsigned long)tsk + THREAD_SIZE - sizeof(struct pt_regs);

	return (struct pt_regs *)p;
}

/*
 * 设置子进程的上下文信息
 */
static int copy_thread(unsigned long clone_flags, struct task_struct *p,
		unsigned long fn, unsigned long arg)
{
	struct pt_regs *childregs;
	
	childregs = task_pt_regs(p);
	memset(childregs, 0, sizeof(struct pt_regs));
	memset(&p->cpu_context, 0, sizeof(struct cpu_context));

	if (clone_flags & PF_KTHREAD) {
		const register unsigned long gp __asm__ ("gp");
		childregs->gp = gp;

		childregs->sstatus = SR_SPP | SR_SPIE;

		//s0是fp,所有这里改为使用s1与s2
		p->cpu_context.s[1] = fn; /* fn */
		p->cpu_context.s[2] = arg;

		p->cpu_context.ra = (unsigned long)ret_from_kernel_thread;
	} else {//clone_flags==0 这是是在S模式(内核态)
		//current是父进程的内核栈tp
		//这里将父进程的中断现场 copy给子进程
		*childregs = *task_pt_regs(current);
		//fork一般是父进程返回pid,子进程返回0。
		//父进程是在invoke_syscall中设置a0
		//子进程是在这里设置
		childregs->a0 = 0;
		//如果clone_flags为0,那么fn参数其实是clone子进程用户栈底(栈的最高地址) -16
		unsigned long usr_sp = fn;
		if (usr_sp)
			childregs->sp = usr_sp;
		//因为我们是从用户态调用clone函数的,所以这里不用清设置childregs->sstatus,用父进程就可以

		p->cpu_context.ra = (unsigned long)ret_from_fork;
	}

	p->cpu_context.sp = (unsigned long)childregs; /* kernel sp */

	return 0;
}

/*
 * fork一个新进程
 * 1. 新建一个task_strut。 分配4KB页面用来存储内核栈,
 * task_struct在栈底。
 * 2. 分配PID
 * 3. 设置进程的上下文
 */
int do_fork(unsigned long clone_flags, unsigned long fn, unsigned long arg)
{
	struct task_struct *p;
	int pid;
	/* 分配内核栈 */
	p = (struct task_struct *)get_free_page();
	if (!p)
		goto error;

	memset(p, 0, sizeof(*p));

	pid = find_empty_task();
	if (pid < 0)
		goto error;

	if (copy_thread(clone_flags, p, fn, arg))
		goto error;

	p->state = TASK_RUNNING;
	p->pid = pid;
	p->counter = (current->counter + 1) >> 1;
	current->counter >>= 1;
	p->need_resched = 0;
	p->preempt_count = 0;
	p->priority = 2;
	total_forks++;
	g_task[pid] = p;
	/* 构建PCB 双向循环链表 */
	SET_LINKS(p);
	/* 加入就绪队列 */
	wake_up_process(p);

	return pid;

error:
	return -1;
}

/* 
 * 因为do_fork后 内核进程第一次执行的是ret_from_kernel_thread,然后执行fn,最后跳转到ret_from_exception执行 
 * 所以可以在fn中完成三个步骤:
 * 1.设置当前进程内核栈的regs->sepc,
 * 2.设置当前进程内核栈的regs->sp,
 * 3 清除当前进程内核栈的regs->sstatus的SPP位
 * 这样在后续从异常返回后(sret),就会在U模式上,sepc处执行代码,此时sp指向用户栈
 */
static void start_user_thread(struct pt_regs *regs, unsigned long pc,
		unsigned long sp)
{
	unsigned long val;
	// raw_local_irq_disable();
	memset(regs, 0, sizeof(*regs));
	regs->sepc = pc;
	regs->sp = sp;

	val = read_csr(sstatus);
	regs->sstatus = val &~ SR_SPP;
	printk("sstatus 0x%llx sp 0x%llx  pc 0x%llx\n", regs->sstatus, regs->sp, regs->sepc);
	// raw_local_irq_enable();
}
/* 利用异常返回U模式时,执行的PC值 */
int move_to_user_space(unsigned long pc)
{
	struct pt_regs *regs;
	unsigned long stack;

	regs = task_pt_regs(current);

	/* 分配用户栈 */
	stack = get_free_page();
	if (!stack)
		return -1;

	memset((void *)stack, 0, PAGE_SIZE);
	
	start_user_thread(regs, pc, stack + PAGE_SIZE);

	return 0;
}
