# cse422_lab2
MODULE DESIGN

	Between the locking and atomic versions of the module, the only differences are in the sieve function. The init 
	functions are straightforward: initialize the global variables and kmalloc() space for the global arrays. We spawn
	num_threads kthreads, and assign each a spot in the counters array by passing the i’th thread a pointer to counters[i] 
	as the only argument. Each kthread
	calls the run() function, which synchronizes at the first barrier, calls sieve and passes &counters[i], synchronizes at the 
	second barrier, and returns after setting the progress variable to DONE. 
	We implemented a single barrier function, which differentiates between before / after the sieve with a single int argument 
	(1 for the first barrier, 2 for the second). In init(), we create two ordinary int variables and initialize them to num_threads.
	They serve as counters of how many threads there are that have not reached the barrier. When each thread arrives at the barrier, 
	it uses a spin_lock to safely decrement the counter corresponding to that barrier, and sets the timestamp if it was the last thread
	to reach the barrier (ie if it was the one to set the counter to zero). If there are still threads that haven’t reached the barrier,
	the program waits in a while loop, calling schedule() as long as the counter != 0. This does create a data race as several threads 
	try to read the counter and others try to write, but because only 1 thread at a time can write, the data race has no effect on the
	function of the method. 
	In both versions of sieve, the thread searches for the lowest number that has not yet been crossed out and crosses out all of its
	multiples. This work is performed in an infinite while loop, only breaking out when we reach the upper bound. At the end of each
	
	iteration (when we’ve finished crossing out all the multiples, the thread calls schedule(). We added this because we noticed 
	that, in the 8-thread runs, half of the threads didn’t end up doing any work at all. For upper bounds up to 1,000,000, four 
	threads did all the work while 4 reported crossing out 0 numbers. This is probably because only four threads can be scheduled 
	at a time, so whichever four threads are on the CPU when the last thread reaches the barrier get to execute immediately and 
	race through all the primes before any of the others even get scheduled. Calls to schedule slowed down performance, but 
	the number of non-primes that each thread crossed out became more even.
	In the locking version, we maintain two spinlocks, one to protect pos, the index of the current prime, and the second to 
	protect the array while we cross out multiples. First, we lock around pos and look for the next non-zero number. Once we’ve 
	found it, we save the value locally and increment pos one more time to give the next thread a place to start. Then, we unlock 
	pos and lock while crossing out multiples. Starting at 2*pos and increasing by pos every time, we cross out each multiple 
	until we reach the end of the array, then unlock and call schedule(). 
	In the atomic version, we made pos an atomic_t along with the values in the array. Now, inside of a while(true) loop, 
	we simply atomic_inc_return(&pos) in a while loop until we find a nonzero value, and then atomic_set every multiple of pos to zero. 

SIEVE OF SUNDARAM

	Our implementation was based of the original locking module. 
	We chose this version because it had the best overall performance between the atomic_t and spin_lock. The algorithm is very 
	different from 
	the sieve of Eratosthenes. We used our num array as a list to generate odd prime factors. In this case this only has to be (n-2)/2 in 
	size. Num arrays is then filled with numbers matching its index. In each thread there is a loop variable i. The variable is 
	initialized to
	the the whatever position the thread hold. The thread then marks off  i + myPos + 2 * (i * myPos) if it is less then the size of the 
	array. The thread then increments the loop variable and repeats the process until the above expression exceeds the size of the array. 
	Once all the threads have finished for the array the actual prime numbers are computed using the integers not marked. This unmarked 
	values are  doubled and the result incremented to produce a prime number. 
