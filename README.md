# MESI-Cache-Simulator

This project is to simulate the workings of a MESI Cache Coherence Protocol for a multicore structure. 
According to Wikipedia, "The MESI protocol is an Invalidate-based cache coherence protocol, and is one of the most common protocols that support write-back caches. 
Write back caches can save a lot on bandwidth that is generally wasted on a write through cache. 
There is always a dirty state present in write back caches that indicates that the data in the cache is different from that in main memory. 
Illinois Protocol requires cache to cache transfer on a miss if the block resides in another cache. 
This protocol reduces the number of Main memory transactions with respect to the MSI protocol. 
This marks a significant improvement in the performance.Tomasulo Algorithm is a pipelining technique 
in processors that uses functional units to provide a more efficient computing speed compare to serial." 
From https://en.wikipedia.org/wiki/MESI_protocol

The simulator would first treat each line of the provided as a reference to memory. Each line of memory reference looks like 
`0 1 123456`. The first number corresponds to the core that it is referencing. The second number corresponds to whether the reference is 
a read (0) or a write (1), the third number corresponds to the address of the memory. Given these memory references, the simulation 
can proceed with the protocol by changing the state of each memory address. Throughout the simulation, important benchmarks and 
statistics are calculated, such as the number of accesses, misses, fetches, writebacks and bus broadcasts. 

Steps to run the simulation: 
1. Use Linux or WSL
2. Run `make` 
3. Run `./sim -n <number of cores> -bs <block size> -us <cache size> traces/<trace_file>`
4. Sample Command `./sim -n 2 -bs 32 -us 16384 traces/fft-2c.trace`

Supported Features
1. Number of cores supported: 		1 to 8 
2. Number of block size supported: 	16 to 32
3. Number of cachce size supported: 	8192 to 16384
4. Trace file included: 
	- spice-1c.trace 
	- fft-2c.trace 
	- fft-4c.trace 
	- fft-8c.trace 
