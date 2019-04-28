/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>
#include <segmem.h>
#include <cr.h>
#include <pagemem.h>
#include <string.h>
#include <intr.h>
#include <task.h>
#include <asm.h>

#define U1_STACK 0x403000
#define U2_STACK 0x408000

#define K1_STACK 0x310000
#define K2_STACK 0x311000

#define PHYS_COUNTER 0x410000
#define KRN_COUNTER  0x3FF000
#define USR1_COUNTER 0x404000
#define USR2_COUNTER 0x409000

#define PGD0_BADDR 0x350000
#define PGD1_BADDR 0x360000
#define PGD2_BADDR 0x370000

extern info_t *info;

// ----------------  DECLARATION DES VARIABLES GLOBALES ---------------- //
seg_desc_t GDT[6];
tss_t      tss;
idt_reg_t  idtr;
uint32_t*  cmpt_addr_kr   = (uint32_t*) KRN_COUNTER;
uint32_t*  __attribute__((section(".usr1_cpt"))) cmpt_addr_usr1 = (uint32_t*) USR1_COUNTER;
uint32_t*  __attribute__((section(".usr2_cpt"))) cmpt_addr_usr2 = (uint32_t*) USR2_COUNTER;

pde32_t*   pgd_kr   = (pde32_t*) PGD0_BADDR;
pde32_t*   pgd_usr1 = (pde32_t*) PGD1_BADDR;
pde32_t*   pgd_usr2 = (pde32_t*) PGD2_BADDR;

task_t     kernel;
task_t     task_user1;
task_t     task_user2;
task_t*    current_task;

// ------------------  DECLARATION DES APPELS SYSTEMES  -------------------- //
void __attribute__((section(".usr2_sys"))) sys_counter(uint32_t* counter)        //interface utilisateur de l'appel système d'affichage
{   
	asm volatile ("mov %0, %%esi" :: "m"(counter));
	asm volatile ("int $80");
}

// -----------------  DECLARATION DES TACHES UTILISATEUR  ------------------ //
void __attribute__((section(".user1"))) write_counter()        //tâche 1, incrémente le compteur dans la mémoire partagée
{
    while(1) { (*cmpt_addr_usr1)++; }
}

void __attribute__((section(".user2"))) print_counter()        //tâche 2, affiche le compteur depuis la mémoire partagée
{   
    while(1) { sys_counter(cmpt_addr_usr2); }
}

// -------------------  DECLARATION DES FONCTIONS NOYAU  ------------------- //
void init_memory()
{
    // SEGMENTATION -----------------------------------------------------
    gdt_reg_t gdtr;

    GDT[0].raw = 0ULL;

    c0_dsc(&GDT[c0_idx]);
    d0_dsc(&GDT[d0_idx]);
    c3_dsc(&GDT[c3_idx]);
    d3_dsc(&GDT[d3_idx]);
    tss_dsc(&GDT[ts_idx], (offset_t)&tss);

    gdtr.desc  = GDT;
    gdtr.limit = sizeof(GDT) - 1;
    set_gdtr(gdtr);

    tss.s0.esp = get_ebp();
	tss.s0.ss = d0_sel;

    set_cs(c0_sel);

    set_ss(d0_sel);
    set_ds(d0_sel);
    set_es(d0_sel);
    set_fs(d0_sel);
    set_gs(d0_sel);

    set_tr(ts_sel);

    // PAGINATION -------------------------------------------------------
    uint32_t* counter_phys = (uint32_t*) PHYS_COUNTER;
    int       ptb_idx;

    //Kernel Pages
    pte32_t* ptb1_kr = (pte32_t*) (PGD0_BADDR + 0x1000);

    for(int i=0;i<1024;i++) {                                   //PT1
        pg_set_entry(&ptb1_kr[i], PG_KRN|PG_RW, i);
    }

    ptb_idx = pt32_idx(KRN_COUNTER);
    pg_set_entry(&ptb1_kr[ptb_idx], PG_KRN|PG_RW, (uint32_t)counter_phys / 0x1000);

    memset((void*)pgd_kr, 0, PAGE_SIZE);                        //PGD
    pg_set_entry(&pgd_kr[0], PG_KRN|PG_RW, page_nr(ptb1_kr));

    //Task1 Pages
    pte32_t* ptb1_usr1 = (pte32_t*) (PGD1_BADDR + 0x1000);
    pte32_t* ptb2_usr1 = (pte32_t*) (PGD1_BADDR + 0x2000);

    for(int i=0;i<1024;i++) {                                   //PT1
        pg_set_entry(&ptb1_usr1[i], PG_KRN|PG_RW, i);
    }

    memset((void*)ptb2_usr1, 0, PAGE_SIZE);                     //PT2
    for(int i=0;i<4;i++) {
        pg_set_entry(&ptb2_usr1[i], PG_USR|PG_RW, i+0x400);
    }

    ptb_idx = pt32_idx(USR1_COUNTER);
    pg_set_entry(&ptb2_usr1[ptb_idx], PG_USR|PG_RW, (uint32_t)counter_phys / 0x1000);           //adresse partagée
    
    memset((void*)pgd_usr1, 0, PAGE_SIZE);                      //PGD
    pg_set_entry(&pgd_usr1[0], PG_USR|PG_RW, page_nr(ptb1_usr1));
    pg_set_entry(&pgd_usr1[1], PG_USR|PG_RW, page_nr(ptb2_usr1));
    
    //Task2 Pages
    pte32_t* ptb1_usr2 = (pte32_t*) (PGD2_BADDR + 0x1000);
    pte32_t* ptb2_usr2 = (pte32_t*) (PGD2_BADDR + 0x2000);
    
    for(int i=0;i<1024;i++) {                                   //PT1
        pg_set_entry(&ptb1_usr2[i], PG_KRN|PG_RW, i);
    }

    memset((void*)ptb2_usr2, 0, PAGE_SIZE);                     //PT2
    for(int i=0;i<4;i++) {
        pg_set_entry(&ptb2_usr2[i+5], PG_USR|PG_RW, i+0x405);
    }
 
    ptb_idx = pt32_idx(USR2_COUNTER);
    pg_set_entry(&ptb2_usr2[ptb_idx], PG_USR, (uint32_t)counter_phys / 0x1000);                 //adresse partagée
    
    memset((void*)pgd_usr2, 0, PAGE_SIZE);                      //PGD
    pg_set_entry(&pgd_usr2[0], PG_USR|PG_RW, page_nr(ptb1_usr2));
    pg_set_entry(&pgd_usr2[1], PG_USR|PG_RW, page_nr(ptb2_usr2));

    //Enable Pagination
    set_cr3((uint32_t)pgd_kr);
    uint32_t cr0 = get_cr0();
    set_cr0(cr0|CR0_PG);
}

void prep_task()
{
    init_krn(&kernel, pgd_kr, &task_user2);
    init_task(&task_user1, (uint32_t) &write_counter, (uint32_t*) K1_STACK, (uint32_t*) U1_STACK, pgd_usr1, &task_user2);
    init_task(&task_user2, (uint32_t) &print_counter, (uint32_t*) K2_STACK, (uint32_t*) U2_STACK, pgd_usr2, &task_user1);

    set_ds(d3_sel);
    set_es(d3_sel);
    set_fs(d3_sel);
    set_gs(d3_sel);

    current_task = &kernel;
    *cmpt_addr_kr = 0;
    current_task->krn_stack = (uint32_t*) get_esp();
}

// -------------------------  DEMARRAGE DE L'OS ------------------------- //
void tp()
{
    init_memory();
    prep_task();
    force_interrupts_on();
    
    while(1);
}
