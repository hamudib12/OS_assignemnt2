#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "uthread.h"

struct uthread uthread[MAX_UTHREADS];
struct uthread *callerThread;
int threadCount = 0;


void scheduler(void) { // scheduler for choosing the threads.
    struct uthread *thread;
    struct uthread *choosedThread = uthread; // thread for max.
    int max = -1;
    for (thread = uthread; thread < &uthread[MAX_UTHREADS]; thread++) {
        if (thread->state == RUNNABLE && (int)(thread->priority) > max) {
            max = (int) (thread->priority);
            choosedThread = thread;
        }
    }
    struct uthread *previous = callerThread;
    callerThread = choosedThread;
    callerThread->state = RUNNING;
    uswtch(&previous->context, &choosedThread->context);
}

void start_scheduler(void){
    struct uthread *thread;
    struct uthread *choosedThread = uthread;
    int max = -1;
    for (thread = uthread; thread < &uthread[MAX_UTHREADS]; thread++) {
        if (thread->state == RUNNABLE && (int) (thread->priority) > max) {
            max = (int) (thread->priority);
            choosedThread = thread;
        }
    }
    thread = choosedThread;
    callerThread = choosedThread;
    callerThread->state = RUNNING;
    ((void (*)())(callerThread->context.ra))();
    exit(0);
}

int uthread_create(void (*start_func)(), enum sched_priority priority) {
    struct uthread *ut;
    int i;
    for (i = 0; i < MAX_UTHREADS; i++) {
        //just if we find a free thread in the ULT table.
        if (uthread[i].state == FREE) {
            ut = &uthread[i];
            memset(&ut->context, 0, sizeof(ut->context));
            ut->priority = priority;
            ut->state = RUNNABLE;
            // Set the 'ra' register to the start_func
            ut->context.ra = (uint64) start_func;
            // Set the 'sp' register to the top of the relevant ustack
            ut->context.sp = (uint64)(ut->ustack + STACK_SIZE);
            threadCount++; // accumulate number of threads in the system, to check their number in the u_exit() function;
            //set a block of memory.
            return 0;
        }
    }
    return -1;
}

void uthread_yield() {
    callerThread->state = RUNNABLE; //maybe there is a problem, cause scheduler may choose this thread again
    scheduler(); //get the next process to work from scheduler.
}

void uthread_exit() {
    callerThread->state = FREE;
    callerThread->priority = 0;
    threadCount -= 1;
    if (threadCount <= 0) { //check if there is just one thread in the system.
        exit(0);
    }
    scheduler();

}

int uthread_start_all(void){
    static int main = 1;
    if(main){
        main = 0;
        start_scheduler();
    }
    return -1;
}

enum sched_priority uthread_set_priority(enum sched_priority priority) {
    enum sched_priority previous = callerThread->priority; //to return the previous priority.
    callerThread->priority = priority;
    return previous;
}

enum sched_priority uthread_get_priority(void) {
    return callerThread->priority;
}


struct uthread *uthread_self(void) {
    return callerThread;
}
