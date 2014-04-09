#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);


void halt_us (void);
void exit_us (void);
pid_t exec_us (const char *cmd_line);
int wait_us (pid_t pid);
bool create_us (const char *file, unsigned initial_size);
bool remove_us (const char *file); 
int open_us (const char *file);
int filesize_us (int fd);
int read_us (int fd, void *buffer, unsigned size);
int write_us (int fd, const void *buffer, unsigned size);
void seek_us (int fd, unsigned position);
unsigned tell_us (int fd);
void close_us (int fd);


#endif /* userprog/syscall.h */
