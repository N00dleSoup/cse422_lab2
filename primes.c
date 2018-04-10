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

static unsigned long long timestamps[3];

#define THREADS_RUNNING 0
#define THREADS_DONE 1
#define DEBUG 0;

atomic_t progress; 
//atomic_t my_barrier_1;
//atomic_t my_barrier_2;

static volatile int my_barrier_1;
DEFINE_SPINLOCK(lock_barrier1);
static volatile int my_barrier_2;
DEFINE_SPINLOCK(lock_barrier2);

DEFINE_SPINLOCK(pos_lock); 
DEFINE_SPINLOCK(arr_lock);//Locks while crossing out

static int run(void * counter); 
static void my_barrier(int which);
static void sieve(unsigned long * counter); 


unsigned long long get_time(void) {
    struct timespec* ts; 
    unsigned long long ns; 
    getnstimeofday(ts);
    ns = (unsigned long long) timespec_to_ns(ts);
	return ns;
}

static int primes_init(void) {
	int i;
	timestamps[0] = get_time();
	my_barrier_1 = num_threads;
	my_barrier_2 = num_threads;
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
	for(i = 0; i < num_threads; ++i) {
		counters[i] = 0;
	}
	for(i = 0; i < upper_bound+1; ++i) {
		nums[i] = i;
	}
	atomic_set(&progress, THREADS_RUNNING);
	
	for(i = 0; i < num_threads; ++i) {
		//Pass an index into counters[] to each thread
		kthread_run(run, counters + i, "sieve_proc_%d",i);
	}
	return 0;
}


static int run(void * counter) {
	my_barrier(1);
	sieve( (unsigned long *) counter);
	my_barrier(2);
	atomic_set(&progress, THREADS_DONE);
	return 0;
}

//my_barrier_1 and 2 originally set to num_threads
//Each call to this function atomically decreases the 
//count by one, until it reaches zero 
//and all threads are freed to move on
static void my_barrier(int which) {
	printk(KERN_DEBUG "start barrier %d\n", which);
	if(which == 1) {
		spin_lock(&lock_barrier1);
		my_barrier_1 -= 1;
		//If this is the last thread to get here, update timestamps
		if(my_barrier_1 == 0) {
			timestamps[1] = get_time();
		}
		spin_unlock(&lock_barrier1);
		while ( my_barrier_1 != 0 ) {
			schedule();
		}
	}
	else if(which == 2) {
		spin_lock(&lock_barrier2);
		my_barrier_2 -= 1;
		if(my_barrier_2 == 0) {
			timestamps[2] = get_time();
		}
		spin_unlock(&lock_barrier2);
		while(my_barrier_1 != 0) {
			schedule();
		}
	}
	printk(KERN_DEBUG "End barrier %d\n", which);
} 

static void sieve(unsigned long * counter) {
	int myPos, i;
	printk(KERN_DEBUG "start sieve with counter %d\n", counter - counters);
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
		++pos; //So the next thread starts looking at higher numbers
		spin_unlock(&pos_lock);
	
		//printk(KERN_DEBUG "Crossing out multiples of %d\n", myPos);
	
		spin_lock(&arr_lock);
		for(i = 2*myPos; i <= upper_bound; i += myPos) {
			nums[i] = 0;
			//printk(KERN_DEBUG "Crossed out %d\n", i);
			(*counter) += 1;
		}
		spin_unlock(&arr_lock);
		schedule();
	}
	printk(KERN_DEBUG "end sieve with counter %d\n", counter - counters);
}

static void primes_exit(void) {
	int i, num_primes, cross_outs;
	num_primes = 0;
	cross_outs = 0;
	printk(KERN_DEBUG "Start exit func\n");
	if(atomic_read(&progress) != THREADS_DONE) {
		printk(KERN_ERR "Threads not finished on exit!\n");
	}	
	for(i = 2; i < upper_bound+1; ++i) {
		if( nums[i] != 0 ) {
			printk(", %d", i);
			num_primes++;
                       if(num_primes % 8 == 0){
                          printk("\n");
                       }
		}
	}
	printk("\nPrimes found: %d, Non-primes found: %lu\n",
			 num_primes, upper_bound - num_primes);
	for(i = 0; i < num_threads; ++i) {
		cross_outs += counters[i];
		printk("Thread %d crossed out %lu non-primes\n", i, counters[i]);
	}
	printk("There were %lu extra cross-outs\n", 
			cross_outs - (upper_bound - num_primes) );
	printk("upper_bound = %lu, num_threads = %lu\n", upper_bound, num_threads);
	printk("Setup time: %llu, processing time: %llu\n", 
			timestamps[1]-timestamps[0], timestamps[2]-timestamps[1]);
	kfree(nums);
	kfree(counters);
}

module_init(primes_init);
module_exit(primes_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Patrick Gardner, Nathan Jarvis, Noah Saffer");
MODULE_DESCRIPTION("Lab 2, a sieve for prime numbers");
