<!-- Uses markdown syntax for neat display at github -->

# Abbey

As a typical example of playing around with code at the first year at Almende I created a threadpool with agents on the level of threads. It is an attempt to misuse setjmp and longjmp to enforce cheap "context switches". Personally I am of the opinion threads are overrated, and we should do much more on the level of processes. Inter-process communication is fast enough for most purposes, and we can use message queues, shared memory, etc. if we need to high-bandwidth communication between processes which you will need for example in an image processing pipeline. And in that case, it even makes sense to do it in the cloud. Just bare with the latency of the uplink and the downlink, but the processing on a server can be blazingly fast.

## Copyrights
The copyrights (2008) belong to:

- Author: Anne van Rossum
- Author: Peet van Toren
- Almende B.V., http://www.almende.com and DO bots B.V., http://www.dobots.nl
- Rotterdam, The Netherlands


