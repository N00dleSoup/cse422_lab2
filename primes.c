#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <asm/atomic.h>
#include <asm/spinlock.h>
#include <linux/slab.h>


static unsigned long num_threads = 1, upper_bound = 10;
module_param(num_threads, ulong, 0644);
module_param(upper_bound, ulong, 0644);

static unsigned long * counters;
static int * nums; //sizeof(nums) = upper_bound + 1
static int pos = 2;

#define THREADS_RUNNING 0
#define THREADS_DONE 1
atomic_t progress; 
atomic_t my_barrier_1;
atomic_t my_barrier_2;

DEFINE_SPINLOCK(pos_lock); 
DEFINE_SPINLOCK(arr_lock);//Locks while crossing out

static int run(void * counter); 
static void my_barrier(int which);
static void sieve(unsigned long * counter); 

static int primes_init(void) {
	if( num_threads < 1 || upper_bound < 2) {
		printk(KERN_ERR "Num_threads must be at least 1, upper_bound at least 2\n");
		counters = NULL;
		nums = NULL;
		upper_bound = 0;
		num_threads = 1;
		atomic_set(&progress, THREADS_DONE);
		return -1;
	}
	atomic_set(&progress, THREADS_RUNNING);
	nums = kmalloc( (upper_bound+1) * sizeof(int), GFP_KERNEL);
	if( !nums ) {
		printk(KERN_ERR "kmalloc failed for nums\n");
		return -1;
	}
	
	counters = kmalloc(num_threads * sizeof(unsigned long), GFP_KERNEL);
	if ( !counters ) {
		printk(KERN_ERR "kmalloc failed for counters\n");
		kfree(nums);
		return -1;
	}
	int i;
	for(i = 0; i < num_threads; ++i) {
		counters[i] = 0;
	}
	for(i = 0; i <= upper_bound+1; ++i) {
		nums[i] = i;
	}
	atomic_set(&progress, THREADS_RUNNING);
	atomic_set(&my_barrier_1, num_threads);
	atomic_set(&my_barrier_2, num_threads);
	//TODO: run
	
	kthread_run(run, counters, "sieve_proc");

	return 0;
}


static int run(void * counter) {
	my_barrier(1);
	sieve( (unsigned long *) counter);
	my_barrier(2);
	atomic_set(&progress, THREADS_DONE);
	return 0;
}

//Atomic ints originally set to num_threads
//Each call to this function decreases the 
//count by one, until it reaches zero 
//and all threads are freed to move on
static void my_barrier(int which) {
	printk(KERN_DEBUG "start barrier %d\n", which);
	if(which == 1) {
		atomic_dec(&my_barrier_1);
		while( atomic_read(&my_barrier_1) ) {
			//No need to call schedule, this one won't take long	
		}
	}
	else {
		atomic_dec(&my_barrier_2);
		while( atomic_read(&my_barrier_2) ){
		//TODO: call schedule() here?
		}
	}
	printk(KERN_DEBUG "End barrier %d\n", which);
} 

static void sieve(unsigned long * counter) {
	printk(KERN_DEBUG "start sieve with counter %lu\n", *counter);
	int myPos, i;
	while(1) {
		spin_lock(&pos_lock);
		if(pos > upper_bound) {
			spin_unlock(&pos_lock);
			return;
		}
		while( nums[pos] == 0 && pos <= upper_bound ) {
			++pos;
		}
		if(pos > upper_bound) {
			spin_unlock(&pos_lock);
			return;
		}
	
		myPos = pos;
		++pos;
		spin_unlock(&pos_lock);
	
		printk(KERN_DEBUG "Crossing out multiples of %d\n", myPos);
	
		spin_lock(&arr_lock);
		for(i = 2*myPos; i <= upper_bound; i += myPos) {
			nums[i] = 0;
			printk(KERN_DEBUG "Crossed out %d\n", i);
			(*counter) += 1;
		}
	
		spin_unlock(&arr_lock);
	}
	printk(KERN_DEBUG "End sieve with counter %lu\n", *counter);
}

static void primes_exit(void) {
	int i, num_primes;
	num_primes = 0;
	printk(KERN_DEBUG "Start exit func\n");
	if(atomic_read(&progress) != THREADS_DONE) {
		printk(KERN_ERR "Threads not finished on exit!\n");
	}	
	for(i = 2; i <= upper_bound+1; ++i) {
		if( nums[i] != 0 ) {
			printk("%d\n", i);
			num_primes++;
		}
	}
	printk("Primes found: %d, Non-primes found: %lu\n",
			 num_primes, upper_bound - num_primes);
	printk("upper_bound = %lu, num_threads = %lu\n", upper_bound, num_threads);
	//TODO: timing
	
	kfree(nums);
	kfree(counters);
}

module_init(primes_init);
module_exit(primes_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Patrick Gardner, Nathan Jarvis, Noah Saffer");
MODULE_DESCRIPTION("Lab 2, a sieve for prime numbers");
