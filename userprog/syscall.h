#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <list.h>

void syscall_init (void);
void exit_us (int status);

struct semaphore sys_sema;
int exec_counter;

struct process
{
	int pid;
	struct list_elem process_elem;
	bool alive;
	int exit_status;
	bool wait; 
};


#endif /* userprog/syscall.h */
