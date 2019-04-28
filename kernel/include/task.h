#ifndef __TASK_H__
#define __TASK_H__

#include <intr.h>
#include <gpr.h>
#include <pagemem.h>

#define FAKE_EBP 0x0;

#define init_krn(_kRn_,_pG_,_ntSk_)                     \
    ({                                                  \
      (_kRn_)->pgd             = (pde32_t*)_pG_;        \
      (_kRn_)->next_task       = (struct task*)_ntSk_;  \
    })


typedef struct task task_t;

struct task {
    uint32_t*  krn_stack;
    uint32_t*  top_stack;
    pde32_t*   pgd;
    task_t*    next_task;
};

void init_task(task_t* task, uint32_t task_eip, uint32_t* kernel_stack_addr, uint32_t* user_stack_addr, pde32_t* task_pgd, task_t* next_task);

#endif
