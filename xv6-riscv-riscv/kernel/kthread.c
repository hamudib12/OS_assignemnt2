#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

extern struct proc proc[NPROC];

// initiate the kthread table, setting all the threads in it are UNUSED, and they point to the process that found them.
void kthreadinit(struct proc *p) {
    initlock(&(p->tid_lock), "next_tid"); // init the lock of the tid.
    for (struct kthread *kt = p->kthread; kt < &p->kthread[NKT]; kt++) {
        initlock(&(kt->t_lock), "t_lock");
        kt->t_state = TUNUSED;
        kt->founder = p; // the process that thread belongs to.
        // WARNING: Don't change this line!
        // get the pointer to the kernel stack of the kthread
        kt->kstack = KSTACK((int) ((p - proc) * NKT + (kt - p->kthread)));
    }
}

// return a pointer for the working thread.
struct kthread *mykthread() {
    push_off();
    struct cpu *c = mycpu();
    struct kthread *kt = 0;
    if(c->thread != 0){ //check if c->thread exist.
        kt = c->thread;
    }
    pop_off();
    return kt;
}

// will use it when we want to get the trap_frame pointer in the alloc_thread function;
struct trapframe *get_kthread_trapframe(struct proc *p, struct kthread *kt) {
    return p->base_trapframes + ((int) (kt - p->kthread));
}



// get the unique tid and make new one.
int alloctpid(struct proc *p) {
    int tpid;
    acquire(&p->tid_lock);
    tpid = p->next_tid;
    p->next_tid = p->next_tid + 1; // for the next thread, to have its unique id.
    release(&p->tid_lock);
    return tpid;
}

// allocate new thread.
struct kthread *allocthread(struct proc *p, uint64 kforkret) { // same as allocproc.
    struct kthread *kt;
    for (kt = p->kthread; kt < &p->kthread[NKT]; kt++) {
        acquire(&kt->t_lock);
        if (kt->t_state == TUNUSED) { // look for unused entry.
            goto found;
        } else {
            release(&kt->t_lock);
        }
    }
    return 0;

    found:
        kt->tid = alloctpid(p);
        kt->t_state = TUSED;

        if ((kt->trapframe = get_kthread_trapframe(p, kt)) == 0) { // get the trap_frame.
            freethread(kt);
            release(&kt->t_lock);
            return 0;
        }

        memset(&kt->context, 0, sizeof(kt->context)); // init context.
        kt->context.ra = kforkret;
        kt->context.sp = kt->kstack + PGSIZE;
        return kt;

}

void freethread(struct kthread *t) { // assigns all the states to 0;
    t->trapframe = 0;
    t->tid = 0;
    t->t_chan = 0;
    t->t_killed = 0;
    t->t_xstate = 0;
    t->t_state = TUNUSED;
}
