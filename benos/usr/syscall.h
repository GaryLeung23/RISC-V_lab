#ifndef _BENOS_USR_SYSCALL_H_
#define  _BENOS_USR_SYSCALL_H_

//第一个参数必定是syscall号
extern unsigned long syscall(int nr, ...);

unsigned long malloc(void);

extern int __clone(int (*fn)(void *arg), void *child_stack,
		int flags, void *arg);

int clone(int (*fn)(void *arg), void *child_stack,
		int flags, void *arg);

#endif
