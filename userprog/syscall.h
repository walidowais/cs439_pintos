#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
void exit_us (int status);

struct semaphore sys_sema;
int exec_counter;


#endif /* userprog/syscall.h */
