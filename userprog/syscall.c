#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"



static void syscall_handler (struct intr_frame *);

int fd = 2;

struct filez {
	int fd;
};


/*Write "size" bytes from buffer to open file "fd"
Returns the number of bytes actually written, which may/may not
be less than size that's being passed in

If you reach the end of file, return 0; indicates 
no more bytes could be written

fd-1 writes to the console. Your code to write to console should 
write all of "buffer" in one call to putbuf(), at least as long as
"size" is not bigger than a few hundred bytes.s*/
static int write_us (int fd, const void *buffer, unsigned size){
	//printf("fd: 0x%0x   buffer: 0x%0x    size: 0x%0x\n", fd, buffer, size);

	if(buffer == NULL || !is_user_vaddr(buffer)){
		//invalid pointer
		printf("Invalid Pointer\n");
		thread_exit();
	}

	void *page = pagedir_get_page((thread_current()->pagedir), buffer);
	if(page == NULL){
		//unmapped page
		thread_exit();
	}

	//valid pointer

	int bytes_written = 0;
	//Check if the pointers are correct
	if (fd == 1){
		putbuf(buffer, size);
		bytes_written = size;
	} else {
		printf("NOT IMPLEMENTED YET.\n");
	}

	return bytes_written;
}

/*
void
putbuf (const char *buffer, size_t n) 
{
  acquire_console ();
  while (n-- > 0)
    putchar_have_lock (*buffer++);
  release_console ();
}
*/

/*Terminates current user program, returning status to the kernel
Note: If the process's parent waits for it, this is the status that
will be returned. 
--Status of 0 indicates success
--Nonzero values indicate errors*/
static void exit_us (int status){
	char *save_ptr;
	struct thread *cur = thread_current();
	
	printf ("%s: exit(%d)\n", strtok_r(cur->name, " ", &save_ptr), status);
  	sema_up(&cur->sema_alive);
  	thread_exit();
}

/*Opens file called "file"
Returns nonnegative integer handle called "fd" 
Returns -1 if file could not be opened

Note: fd's 0 and 1 are reserved for the console.
	fd-0_us (STDIN_FILENO) is standard input
	fd-1_us (STDOUT_FILENO) is standard output
File descriptors are not inherited by child processes
When a single file is opened more than once, each "open" call
returns a new "fd"*/
static int open_us (const char *file){	
	return -1; //file could not be opened
}


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  uint32_t *p = f->esp;
  // printf("system call!\n");
  int status;

  switch(*p){
	  
	  case SYS_HALT:
	  	// printf("SYS_HALT\n");
	  	shutdown_power_off();
	  	break;
	  
	  case SYS_EXIT:
  		// printf("SYS_EXIT\n");
  		exit_us(*(p+1));
	  	break;
	  
	  case SYS_EXEC:
	  	/*Runs executable passed in through cmd_line, passing any given arguments
		-Returns new process's pid
		-Returns -1_us (not a valid pid) if program cannot load or run for any reason
		Cannot return from exec until it knows whether child process successfully loaded executable
		Note: Use appropriate synchronization*/
	  	// printf("SYS_EXEC\n");
	  	break;
	  
	  case SYS_WAIT:
	  	// printf("SYS_WAIT\n");
	  	break;
	  
	  case SYS_CREATE:
	  	/*Creates a new file called "file" with size of "initial_size"
		Returns true if successful, false otherwise
		-Does not open the file after created*/
	  	// printf("SYS_CREATE\n");
	  	break;

	  case SYS_REMOVE:
	  	/*Deletes file called "file" 
		Returns true if successful, false otherwise
		-File can be removed regardless of if it's open or closed*/
	  	// printf("SYS_REMOVE\n");
	  	break;

	  case SYS_OPEN:
	  	// printf("SYS_OPEN\n");
	  	break;

	  case SYS_FILESIZE:
	  	/*Returns the size in bytes of the file open as "fd"*/
	  	// printf("SYS_FILESIZE\n");
	  	break;

	  case SYS_READ:
	  	/*Reads "size" bytes from the open file as "fd" into the buffer
		Returns number of bytes actually read_us (0 at the end of file)
		Returns -1 if file could not be read.
		Note: fd-0 reads from keyboard using "input_getc()*/ 
	  	// printf("SYS_READ\n");
	  	break;

	  case SYS_WRITE:
	  	// printf("SYS_WRITE\n");
	  	f->eax = write_us(*(p+1), *(p+2), *(p+3));
	  	break;

	  case SYS_SEEK:
	  	/*Changes the next byte to be read/written in open file "fd" to "position"
		Position 0 = start of the file

		Seeking past the end of file results in error
		Later read obtains 0 bytes, indicating end of file
		Later write extends the file, filling unwritten gaps with 0s*/
	  	// printf("SYS_SEEK\n");
	  	break;

	  case SYS_TELL:
	  	/*Returns the position of next byte to be read or written in open file "fd"*/
	  	// printf("SYS_TELL\n");
	  	break;

	  case SYS_CLOSE:
	  	/*Close file, "fd"
		Exiting or terminating process implicity closes all of its file descriptors

		Idea: Iterate through all "fd"s and call close?*/
	  	// printf("SYS_CLOSE\n");
	  	break;

	  default:
	  	ASSERT(false) // There should be no default case
	  	break;
	}

}


