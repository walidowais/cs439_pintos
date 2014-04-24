#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "threads/palloc.h"
#include "filesys/filesys.h"
#include "filesys/file.h"


static void syscall_handler (struct intr_frame *);
static bool is_valid (void *p);
static int write_us (int fd, const void *buffer, unsigned size);
static void exit_us (int status);
static bool create_us (const char *file, unsigned initial_size);
static bool remove_us (const char *file);
static int open_us (const char *file);	
static int filesize_us(int fd);
static void close_us(int fd);
static void seek_us(int fd, unsigned position);
static unsigned tell_us(int fd);
static int exec_us(const char *cmd_line);
static int wait_us(int pid);
int super_value; 

struct process
{
	int pid;
	struct thread p_thread;
	bool alive;

};


//Checks whether a given pointer is valid or not
static bool is_valid (void *p){
	bool valid = true;

	if (p == NULL || !is_user_vaddr(p)){
		valid = false;
		return valid;
	}
	void *page = pagedir_get_page((thread_current()->pagedir), p);
	if (page == NULL){
		valid = false;
		return valid;
	}
	return valid;
}


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
	if(!is_valid(buffer)){
		exit_us(-1);
	}

	if(fd <= 0){
		exit_us(-1);
	}


	int bytes_written = 0;
	//Check if the pointers are correct
	if (fd == 1){
		putbuf(buffer, size);
		bytes_written = 0;
	} else {
		bool found = false;

		struct thread *cur = thread_current();
		struct list_elem *e;
		
		if (cur->file == NULL)
			exit_us(0);
		
	  	for (e = list_begin (&cur->fd_list); (e != list_end (&cur->fd_list) && !found);
	    e = list_next (e)){
	  		struct file_holder *f = list_entry (e, struct file_holder, file_elem);
	  		
	  		if(f->fd == fd){
	  			found = true;
	  			bytes_written = file_write(f->file, buffer, size);
	  		}
		}
	}

	return bytes_written;
}


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
  	cur->parent_thread->exit_status = status;
	// super_value = status;
  	thread_exit();
}


/*Creates a new file called "file" with size of "initial_size"
Returns true if successful, false otherwise
-Does not open the file after created*/
static bool create_us (const char *file, unsigned initial_size){
	if(!is_valid(file)){
		exit_us(-1);
	}
	return filesys_create (file, initial_size); 
}


/*Deletes file called "file" 
Returns true if successful, false otherwise
-File can be removed regardless of if it's open or closed*/
// printf("SYS_REMOVE\n");
static bool remove_us (const char *file){
	if(!is_valid(file)){
		exit_us(-1);
	}
	return filesys_remove (file); 
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
	if(!is_valid(file)){
		exit_us(-1);
	}

	struct file *fp = filesys_open(file);
	if(fp == NULL){
		 return -1;
	}

	struct thread *cur = thread_current();
	cur->file = file;

	struct file_holder *fh;
	fh = palloc_get_page(PAL_USER);

	fh->file = fp;
	fh->fd = cur->fd_next;
	cur->fd_next++;

	// if(executable){
	// 	file_deny_write(fh->file);
	// }

	list_push_back(&thread_current()->fd_list, &fh->file_elem);

	return fh->fd;
}


/*Returns the size in bytes of the file open as "fd"*/
static int filesize_us(int fd){

	if(fd <= 1){
		exit_us(-1);
	}

	bool found = false;

	struct thread *cur = thread_current();
	struct list_elem *e;
	
  	for (e = list_begin (&cur->fd_list); (e != list_end (&cur->fd_list) && !found);
    e = list_next (e)){
  		struct file_holder *f = list_entry (e, struct file_holder, file_elem);
  		
  		if(f->fd == fd){
  			found = true;
  			return file_length(f->file);
  		}
	}

	return -1;	
}


/*Close file, "fd"
Exiting or terminating process implicity closes all of its file descriptors
Idea: Iterate through all "fd"s and call close?*/
static void close_us(int fd){

	if(fd <= 1){
		exit_us(-1);
	}

	bool found = false;

	struct thread *cur = thread_current();
	struct list_elem *e;

  	for (e = list_begin (&cur->fd_list); (e != list_end (&cur->fd_list) && !found);
    e = list_next (e)){
  		struct file_holder *f = list_entry (e, struct file_holder, file_elem);
  		
  		if(f->fd == fd){
  			found = true;
  			file_close(f->file);
  			list_remove(e);
  		}
	}

	if(!found){
		exit_us(-1);
	}
}

static int wait_us(int pid){
	struct thread *cur = thread_current();
	if (cur->wait == 1)
		return -1;
	cur->wait = 1;

  return super_value;
}

static void exec_helper(int pid){
	 struct list_elem *e;
	 struct thread *child_thread;
	 struct thread *cur = thread_current();
	 cur->wait = 0;
	 bool found = false;
	 for (e = list_begin (&cur->kid_list); (!found && e != list_end (&cur->kid_list));
	       e = list_next (e))
	 {
	   struct thread *t = list_entry (e, struct thread, kid_elem);
	   if(t->tid == pid){
	     list_remove(&t->kid_elem);
	     child_thread = t;
	     found = true;
	   }
	 }

	 if(!found){
	   super_value = -1;
	 }

	 sema_down(&child_thread->sema_alive);

	 super_value = cur->exit_status;
}

/*Changes the next byte to be read/written in open file "fd" to "position"
Position 0 = start of the file
Seeking past the end of file results in error
Later read obtains 0 bytes, indicating end of file
Later write extends the file, filling unwritten gaps with 0s*/
static void seek_us(int fd, unsigned position){
	if(fd <= 1){
		exit_us(-1);
	}

	bool found = false;

	struct thread *cur = thread_current();
	struct list_elem *e;

  	for (e = list_begin (&cur->fd_list); (e != list_end (&cur->fd_list) && !found);
    e = list_next (e)){
  		struct file_holder *f = list_entry (e, struct file_holder, file_elem);
  		
  		if(f->fd == fd){
  			found = true;
  			file_seek(f->file, position);
  		}
	}

	if(!found){
		exit_us(-1);
	}
}


/*Returns the position of next byte to be read or written in open file "fd"*/
static unsigned tell_us(int fd){
	if(fd <= 1){
		exit_us(-1);
	}

	bool found = false;

	struct thread *cur = thread_current();
	struct list_elem *e;
	

  	for (e = list_begin (&cur->fd_list); (e != list_end (&cur->fd_list) && !found);
    e = list_next (e)){
  		struct file_holder *f = list_entry (e, struct file_holder, file_elem);
  		
  		if(f->fd == fd){
  			found = true;
  			return file_tell(f->file);
  		}
	}

	if(!found){
		return -1;
	}
}

/*Reads "size" bytes from the open file as "fd" into the buffer
Returns number of bytes actually read_us (0 at the end of file)
Returns -1 if file could not be read.
Note: fd-0 reads from keyboard using "input_getc()*/ 
static int read_us (int fd, const void *buffer, unsigned size){
	//printf("fd: 0x%0x   buffer: 0x%0x    size: 0x%0x\n", fd, buffer, size);
	if(!is_valid(buffer)){
		exit_us(-1);
	}


	if(fd == 1 || fd < 0){
		exit_us(-1);
	}

	int bytes_read = 0;
	//Check if the pointers are correct
	if (fd == 0){
		input_getc();
		bytes_read = 1;
	} else {
		bool found = false;

		struct thread *cur = thread_current();
		struct list_elem *e;
		
	  	for (e = list_begin (&cur->fd_list); (e != list_end (&cur->fd_list) && !found);
	    e = list_next (e)){
	  		struct file_holder *f = list_entry (e, struct file_holder, file_elem);
	  		
	  		if(f->fd == fd){
	  			found = true;
	  			bytes_read = file_read(f->file, buffer, size);
	  		}
		}
	}

	return bytes_read;
}


/*Runs executable passed in through cmd_line, passing any given arguments
-Returns new process's pid
-Returns -1_us (not a valid pid) if program cannot load or run for any reason
Cannot return from exec until it knows whether child process successfully loaded executable
Note: Use appropriate synchronization*/
static int exec_us(const char *cmd_line){
	if(!is_valid(cmd_line)){
		exit_us(-1);
	}
	int pid = process_execute(cmd_line);
	bool found = false;
	struct list_elem *e;
	struct thread *child_thread;
	struct thread *cur = thread_current();

	for (e = list_begin (&all_list); (!found && e != list_end (&all_list));
	e = list_next (e)){
		struct thread *t = list_entry (e, struct thread, allelem);
		if(t->tid == pid){
		  child_thread = t;
		  found = true;
		}
	}
	exec_helper(pid);


	if(!found){
		return -1;
	}


	if(thread_current()->tid != pid)
		list_push_back(&thread_current()->kid_list, &child_thread->kid_elem);

	return pid;
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
  bool valid = true;
  int status;
  sema_init(&sys_sema, 1);
  exec_counter = 0;

  //check stack pointer
  if (!is_valid(p)){
  	exit_us(-1);
  }

  switch(*p){	  
	  case SYS_HALT:

	  	shutdown_power_off();

	  	break;
	  
	  case SYS_EXIT:
	  	if (!is_valid(p+1)){
	  		exit_us(-1);
	  	}

  		exit_us(*(p+1));

	  	break;
	  
	  case SYS_EXEC:
	  	if (!is_valid(p+1)){
	  		exit_us(-1);
	  	}

	  	exec_us(*(p+1));

	  	break;
	  
	  case SYS_WAIT:
	  	if (!is_valid(p+1)){
	  		exit_us(-1);
	  	}

	  	f->eax = wait_us(*(p+1));

	  	// printf("process id %d \n",(int) *(p+1) );

	  	break;
	  
	  case SYS_CREATE:
	  	if (!is_valid(p+1) || !is_valid(p+2)){
	  		exit_us(-1);
	  	}

	  	f->eax = create_us(*(p+1), *(p+2));

	  	break;

	  case SYS_REMOVE:
	  	if (!is_valid(p+1)){
	  		exit_us(-1);
	  	}

	  	f->eax = remove_us(*(p+1));

	  	break;

	  case SYS_OPEN:
	  	if (!is_valid(p+1)){
	  		exit_us(-1);
	  	}

	  	f->eax = open_us(*(p+1));

	  	break;

	  case SYS_FILESIZE:
	  	if (!is_valid(p+1)){
	  		exit_us(-1);
	  	}

	  	f->eax = filesize_us(*(p+1));

	  	break;

	  case SYS_READ:
	  	if (!is_valid(p+1) || !is_valid(p+2) || !is_valid(p+3)){
	  		exit_us(-1);
	  	}

	  	f->eax = read_us(*(p+1), *(p+2), *(p+3));

	  	break;

	  case SYS_WRITE:
	  	if (!is_valid(p+1) || !is_valid(p+2) || !is_valid(p+3)){
	  		exit_us(-1);
	  	}

	  	f->eax = write_us(*(p+1), *(p+2), *(p+3));

	  	break;

	  case SYS_SEEK:
	  	if (!is_valid(p+1) || !is_valid(p+2)){
	  		exit_us(-1);
	  	}

	  	seek_us(*(p+1), *(p+2));

	  	break;

	  case SYS_TELL:
	  	if (!is_valid(p+1)){
	  		exit_us(-1);
	  	}

	  	f->eax = tell_us(*(p+1));

	  	break;

	  case SYS_CLOSE:
	  	if (!is_valid(p+1)){
	  		exit_us(-1);
	  	}

	  	close_us(*(p+1));

	  	break;

	  default:
	  // There should be no default case
	  	ASSERT(false) 
	  	break;
	}
}
























//static int wait_us(int pid){
	 
// 	int x = process_wait(pid);
// 	struct list_elem *e;
// 	struct thread *cur = thread_current();
// 	bool found = false;
	
// 	for (e = list_begin (&all_list); (!found && e != list_end (&all_list));
// 	e = list_next (e)){
// 		struct thread *t = list_entry (e, struct thread, allelem);
// 		if(t->tid == pid){
// 		  printf("helloooooo\n");
// 		  return t->exit_status;
// 		}
// 	}

// 	return x;
// }
