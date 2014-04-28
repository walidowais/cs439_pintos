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
#include "threads/malloc.h"

static struct lock rw_lock;
static void syscall_handler (struct intr_frame *);
static bool is_valid (void *p);
static int write_us (int fd, const void *buffer, unsigned size);
static bool create_us (const char *file, unsigned initial_size);
static bool remove_us (const char *file);
static int open_us (const char *file);	
static int filesize_us(int fd);
static void close_us(int fd);
static void seek_us(int fd, unsigned position);
static unsigned tell_us(int fd);
static int exec_us(const char *cmd_line);
static int wait_us(int pid);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}


/*A huge ass switch case for all the system calls.*/
static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  lock_init (&rw_lock);
  uint32_t *p = f->esp;
  bool valid = true;
  int status;
  sema_init(&sys_sema, 1);
  exec_counter = 0;

  if (!is_valid(p)){
  	exit_us(-1);
  }

  /* We increment our pointer to take into account the parameters of each function.*/
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

	  	f->eax = exec_us(*(p+1));

	  	break;
	  
	  case SYS_WAIT:
	  	if (!is_valid(p+1)){
	  		exit_us(-1);
	  	}

	  	f->eax = wait_us(*(p+1));

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
	  	/*I hope to god this never happens.*/
	  	ASSERT(false) 
	  	break;
	}
}


/*Checks whether the address is valid.*/
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

/*Writing to a buffer.*/
static int write_us (int fd, const void *buffer, unsigned size){
	if(!is_valid(buffer)){
		exit_us(-1);
	}

	/*Can't write to stdin, aka 0.*/
	if(fd <= 0){
		exit_us(-1);
	}

	int bytes_written = 0;

	/*Writing to stdout.*/
	if (fd == 1){
		putbuf(buffer, size);
		bytes_written = 0;
	} 

	else {
		bool found = false;

		struct thread *cur = thread_current();
		struct list_elem *e;
		
		if (cur->file == NULL)
			exit_us(0);
		
		/*Traversing through our list of file descriptors.*/
	  	for (e = list_begin (&cur->fd_list); (e != list_end (&cur->fd_list) && !found);
	    e = list_next (e)){
	  		struct file_holder *f = list_entry (e, struct file_holder, file_elem);
	  		
	  		/*If it's never found, we return 0 bytes written.*/
	  		if(f->fd == fd){
	  			/*Found? Write to the buffer file.*/
	  			found = true;
	  			bytes_written = file_write(f->file, buffer, size);
	  		}
		}
	}

	return bytes_written;
}


/*Terminates current user program, returning status to the kernel.*/
 void exit_us (int status){
/*Oh my god this code seriously took forever.*/
	char *save_ptr;
	struct thread *cur = thread_current();
	// printf("Status: %d\n", status);
	printf ("%s: exit(%d)\n", strtok_r(cur->name, " ", &save_ptr), status);
	bool found = false;
	struct list_elem *e;

	/*Traverse the list to find the kid process.*/
	for(e = list_begin(&cur->parent_thread->kid_list); (e != list_end(&cur->parent_thread->kid_list) && !found);
		e = list_next(e)){

		struct process *p = list_entry(e, struct process, process_elem);
		/*Setting status and metadata for the kid process.*/
		if(p->pid == cur->tid){
			p->alive = false;
			p->exit_status = status;
			found = true;
		}
	}

	/*Hopefully this doesn't happen...*/
	if(!found)
		printf("process isn't found\n");
	
	/*Gotta wake up from wait.*/
	sema_up(&cur->parent_thread->sema_alive);

	thread_exit();
}


/*Creates a new file with size of initial_size.*/
static bool create_us (const char *file, unsigned initial_size){
	/*Validity check as usual.*/
	if(!is_valid(file)){
		exit_us(-1);
	}

	return filesys_create (file, initial_size); 
}


/*Deletes file.*/
static bool remove_us (const char *file){
	if(!is_valid(file)){
		exit_us(-1);
	}

	/*Well, this is pretty simple to understand.*/
	return filesys_remove (file); 
}


/*Opens file.*/
static int open_us (const char *file){	
	if(!is_valid(file)){
		exit_us(-1);
	}

	/*Grabs the file pointer.*/
	struct file *fp = filesys_open(file);
	if(fp == NULL){
		 return -1;
	}

	struct thread *cur = thread_current();
	cur->file = file;

	struct file_holder *fh;
	fh = palloc_get_page(PAL_USER);

	/*Adding to list of files.*/
	fh->file = fp;
	fh->fd = cur->fd_next;
	cur->fd_next++;

	/*Each process has its own file descriptor list.*/
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


/*Close file descriptor.*/
static void close_us(int fd){

	/*Can't close stdin/stdout.*/
	if(fd <= 1){
		exit_us(-1);
	}

	bool found = false;

	struct thread *cur = thread_current();
	struct list_elem *e;

	/*Traversing through the list of file descriptors... as usual.*/
  	for (e = list_begin (&cur->fd_list); (e != list_end (&cur->fd_list) && !found);
    e = list_next (e)){
  		struct file_holder *f = list_entry (e, struct file_holder, file_elem);
  		
  		if(f->fd == fd){
  			found = true;
  			file_close(f->file);
  			list_remove(e);
  		}
	}

	/*If not found, then you want to exit, since the fd is invalid.*/
	if(!found){
		exit_us(-1);
	}
}


/*Used a process because exit_us frees the thread;needed struct to hold metadata.*/
static int wait_us(int pid){

	struct thread *cur = thread_current();	
	bool found = false;
	struct list_elem *e;

	/*Metadata for the child thread*/
	struct process *kiddo;
	for(e = list_begin(&cur->kid_list); (e != list_end(&cur->kid_list) && !found);
		e = list_next(e)){

		struct process *p = list_entry(e, struct process, process_elem);
		if(p->pid == pid){
			kiddo = p;			
			found = true;
		}
	}
	
	/*If pid isn't found, then you're out.*/
	if(!found){
		return -1;
	}

	/*So that we don't call wait on same pid twice.*/
	if(kiddo->wait == 1){
		return -1;
	}

	/*Metadata for the threads.*/
	kiddo->wait = 1; 

	/*In the case that the kid is still running.*/
	if(kiddo->alive == true)
		sema_down(&cur->sema_alive);

	return kiddo->exit_status;
}


/*Changes the next byte to be read/written in fd to position.*/
static void seek_us(int fd, unsigned position){

	/*Checking for stdin/stdout case.*/
	if(fd <= 1){
		exit_us(-1);
	}

	bool found = false;

	struct thread *cur = thread_current();
	struct list_elem *e;

	/*Traversing through the list.*/
  	for (e = list_begin (&cur->fd_list); (e != list_end (&cur->fd_list) && !found);
    e = list_next (e)){
  		struct file_holder *f = list_entry (e, struct file_holder, file_elem);
  		
  		/*If we can find the file descriptor, then do it.*/
  		if(f->fd == fd){
  			found = true;
  			file_seek(f->file, position);
  		}
	}

	if(!found){
		exit_us(-1);
	}
}


/*Returns the position of next byte to be read/written in fd.*/
static unsigned tell_us(int fd){
	/*Account for stdin/stdout cases.*/
	if(fd <= 1){
		exit_us(-1);
	}

	bool found = false;

	struct thread *cur = thread_current();
	struct list_elem *e;
	
	/*Traversing through our file descriptors.*/
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

/*Reads "size" bytes from the open file as "fd" into the buffer.*/ 
static int read_us (int fd, const void *buffer, unsigned size){
	if(!is_valid(buffer)){
		exit_us(-1);
	}

	/*Fd can't be 1 because stdout. */
	if(fd == 1 || fd < 0){
		exit_us(-1);
	}

	int bytes_read = 0;

	/*But you can read from stdin!*/
	if (fd == 0){
		input_getc();
		bytes_read = 1;
	} 

	else {
		bool found = false;

		struct thread *cur = thread_current();
		struct list_elem *e;
		
		/*Traversing the list to find our file.*/
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


/*Executes a process and waits on it until finishes.*/
static int exec_us(const char *cmd_line){
	if(!is_valid(cmd_line)){
		exit_us(-1);
	}

	/*Call process execute for loading.*/
	int pid = process_execute(cmd_line);
	bool found = false;
	struct list_elem *e;
	struct thread *child_thread;
	struct thread *cur = thread_current();
	sema_down(&cur->sema_exec);

	/*If the load fails, we want to return -1.*/
	if(cur->load_success == false){
		return -1;
	}

	/*Setting attributes for the metadata.*/
	struct process *p = malloc(sizeof(struct process));
	p->alive = true;
	p->pid = pid;
	p->exit_status = -23;
	p->wait = false;


	list_push_back(&cur->kid_list, &p->process_elem);

	return pid;
}



