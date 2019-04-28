#include <task.h>
#include <intr.h>
#include <gpr.h>
#include <pagemem.h>

extern void resume_from_intr();

void init_task(task_t* task, uint32_t task_eip, uint32_t* kernel_stack_addr, uint32_t* user_stack_addr, pde32_t* task_pgd, task_t* next_task)
{                                                                                                 
	task->krn_stack                           = kernel_stack_addr + PG_4K_SIZE;
	task->top_stack                           = (uint32_t*)((int_ctx_t*) task->krn_stack - 1);
	((int_ctx_t*)task->top_stack)->eip.raw    = task_eip;
	((int_ctx_t*)task->top_stack)->cs.raw     = c3_sel;
	((int_ctx_t*)task->top_stack)->eflags.raw = EFLAGS_IF;
	((int_ctx_t*)task->top_stack)->esp.raw    = (uint32_t) user_stack_addr + PG_4K_SIZE;
	((int_ctx_t*)task->top_stack)->ss.raw     = d3_sel;
	*(--(task)->top_stack)                    = (offset_t) resume_from_intr;
	*(--(task)->top_stack)                    = FAKE_EBP;
	task->pgd                                 = task_pgd;
	task->next_task                           = next_task;
	
}
