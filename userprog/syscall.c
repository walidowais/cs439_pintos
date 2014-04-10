#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  uint32_t *p = f->esp;
  printf("system call!!!");

  switch(*p){
  	/*calls "shutdown_power_off()".*/
	  case SYS_HALT:
	  	break;
	  case SYS_EXIT:
	  	break;
	  case SYS_EXEC:
	  	break;
	  case SYS_WAIT:
	  	break;
	  case SYS_CREATE:
	  	break;
	  case SYS_REMOVE:
	  	break;
	  case SYS_OPEN:
	  	break;
	  case SYS_FILESIZE:
	  	break;
	  case SYS_READ:
	  	break;
	  case SYS_WRITE:
	  	break;
	  case SYS_SEEK:
	  	break;
	  case SYS_TELL:
	  	break;
	  case SYS_CLOSE:
	  	break;
	  default:
	  	ASSERT(false) // There should be no default case
	  	break;
	}
  thread_exit ();
}

// /*calls "shutdown_power_off()".*/
// void halt_us (void)
// {
// 	shutdown_power_off();
// }

// /*Terminates current user program, returning status to the kernel
// Note: If the process's parent waits for it, this is the status that
// will be returned. 
// --Status of 0 indicates success
// --Nonzero values indicate errors*/
// void exit_us (void)
// {

// }

// /*Runs executable passed in through cmd_line, passing any given arguments
// -Returns new process's pid
// -Returns -1_us (not a valid pid) if program cannot load or run for any reason
// Cannot return from exec until it knows whether child process successfully loaded executable
// Note: Use appropriate synchronization*/
// int exec_us (const char *cmd_line)
// {
// 	return -1;
// }

// int wait_us (int pid)
// {
// 	return -1;
// }

// /*Creates a new file called "file" with size of "initial_size"
// Returns true if successful, false otherwise
// -Does not open the file after created*/
// bool create_us (const char *file, unsigned initial_size)
// {
// 	return false;
// }

// /*Deletes file called "file" 
// Returns true if successful, false otherwise
// -File can be removed regardless of if it's open or closed*/
// bool remove_us (const char *file) 
// {
// 	return false;
// }

// /*Opens file called "file"
// Returns nonnegative integer handle called "fd" 
// Returns -1 if file could not be opened

// Note: fd's 0 and 1 are reserved for the console.
// 	fd-0_us (STDIN_FILENO) is standard input
// 	fd-1_us (STDOUT_FILENO) is standard output
// File descriptors not inherited by child processes
// When a single file is opened more than once, each "open" call
// returns a new "fd"*/
// int open_us (const char *file)
// {
// 	return -1;
// }

// /*Returns the size in bytes of the file open as "fd"*/
// int filesize_us (int fd)
// {
// 	return -1;
// }

// /*Reads "size" bytes from the open file as "fd" into the buffer
// Returns number of bytes actually read_us (0 at the end of file)
// Returns -1 if file could not be read.
// Note: fd-0 reads from keyboard using "input_getc()*/ 
// int read_us (int fd, void *buffer, unsigned size)
// {
// 	return -1;
// }

// /*Write "size" bytes from buffer to open file "fd"
// Returns the number of bytes actually written, which may/may not
// be less than size that's being passed in

// If you reach the end of file, return 0; indicates 
// no more bytes could be written

// fd-1 writes to the console. Your code to write to console should 
// write all of "buffer" in one call to putbuf(), at least as long as
// "size" is not biffer than a few hundred bytes.s*/
// int write_us (int fd, const void *buffer, unsigned size)
// {
// 	return -1;
// }

// /*Changes the next byte to be read/written in open file "fd" to "position"
// Position 0 = start of the file

// Seeking past the end of file results in error
// Later read obtains 0 bytes, indicating end of file
// Later write extends the file, filling unwritten gaps with 0s*/
// void seek_us (int fd, unsigned position)
// {

// }

// /*Returns the position of next byte to be read or written in open file "fd"*/
// unsigned tell_us (int fd)
// {
// 	return 23;
// }

// /*Close file, "fd"
// Exiting or terminating process implicity closes all of its file descriptors

// Idea: Iterate through all "fd"s and call close?*/
// void close_us (int fd)
// {

// }