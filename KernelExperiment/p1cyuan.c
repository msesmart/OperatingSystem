#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include "list.h"

struct birthday{
	int day;
	int month;
	int year;
	struct list_head list;
};
static LIST_HEAD(birthday_list);

/* This function is called when the module is loaded. */
int simple_init(void){
    printk(KERN_INFO "Loading Module\n");
	struct birthday *person;
	int i=0;
	printk(KERN_INFO "create linked list with 5 birthday elements...\n");
	while(i<5){
		person=kmalloc(sizeof(*person),GFP_KERNEL);
		person->day=2+i; person->month=8+i; person->year=1995+i;
		INIT_LIST_HEAD(&person->list);
		list_add_tail(&person->list,&birthday_list);
		i++;
	}
	printk(KERN_INFO "# day-month-year \n"); i=1;
	list_for_each_entry(person,&birthday_list,list){
		printk(KERN_INFO "%d  %d-%d-%d \n",i++,person->day,person->month,person->year);
	}
	printk(KERN_INFO "create linked list finished\n");
    return 0;
}

/* This function is called when the module is removed. */
void simple_exit(void) {
	int i=1; struct birthday *ptr, *next;
	printk(KERN_INFO "Removing Module\n");
	printk(KERN_INFO "delete linked list...\n");
	list_for_each_entry_safe(ptr,next,&birthday_list,list){
		/* on each iteration ptr points to the next birthday struct */
		list_del(&ptr->list);
		kfree(ptr);
		printk(KERN_INFO "#%d element removed \n",i++);
	}
	printk(KERN_INFO "delete linked list finished\n");
}

/* Macros for registering module entry and exit points. */
module_init( simple_init );
module_exit( simple_exit );

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple Module");
MODULE_AUTHOR("SGG");

