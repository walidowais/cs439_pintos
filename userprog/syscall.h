#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);

struct semaphore sys_sema;
int exec_counter;


#endif /* userprog/syscall.h */
