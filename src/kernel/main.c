#include <bwio.h>
#include <def.h>
#include <panic.h>
#include <req.h>
#include <td.h>
#include <user/task.h>
#include <scheduler.h>

// Forward decls
const char* buildstr(void);
void initMain(void);

// First task must be created by hand.
void initTask1(struct Td *td, void *stack) {
    td->tid = 1;
    td->ptid = 1;
    td->pri = 3;
    td->nextReady = 0;
    td->sendReady = 0;
    td->state = READY;
    td->sp = stack;
}

void initStack(const void *entrypoint, void **sp) {
    unsigned *word = *sp;
    *(word++) = 0; // r0
    *(word++) = 0; // r1
    *(word++) = 0; // r2
    *(word++) = 0; // r3
    *(word++) = 0; // r4
    *(word++) = 0; // r5
    *(word++) = 0; // r6
    *(word++) = 0; // r7
    *(word++) = 0; // r8
    *(word++) = 0; // r9
    *(word++) = 0; // r10/sl
    *(word++) = 0; // r11/fp
    *(word++) = 0; // r12/ip
    //*(word++) = 0; // r14/lr  TODO: what should happen after a task returns?
    *(word++) = (unsigned)entrypoint; // r15/pc
    // TODO: process status register
    *sp = (void*)word;
}

void svcHandle(unsigned id) {
    // SWI immediate value is only 24bits.
    id &= 0xffffff;
    bwprintf(COM2, "HANDLE %u\r\n", id);
    PANIC("HANDLE");
}

int main() {
    // Print the build string (date + time).
    bwsetspeed(COM2, 115200);
    bwsetfifo(COM2, OFF);
    bwprintf(COM2, "%s\r\n", buildstr());

    static char userStacks[NUM_TD][STACK_SZ];
    struct Td tds[NUM_TD];
    struct Scheduler scheduler;

    initTds(tds);
    initTask1(&tds[0], userStacks[0]);
    initStack(initMain, &tds[0].sp);
    initScheduler(&scheduler);
    readyProcess(&scheduler, &tds[0]);

    volatile void **SVC_VECTOR = (void*)0x28;
    void kernel_entry(void);
    *SVC_VECTOR = &kernel_entry;

    for (int i = 0; i < 4; ++i) {
        struct Td* active = getNextProcess(&scheduler);
        bwprintf(COM2, "Context switching to TID %d\r\n", active->tid);

        // Context switch
        asm volatile (
            // Copy stack pointer from active->sp to sp register.
            "mov sp, %0\n\t"
            // Pop the rest of the registers off of the stack, including pc.
            // LDMEA = LoaD Multiple from Empty Ascending stack
            "ldmea sp!, {r0-r12,pc}" // TODO: works, but might not conform to gcc abi
            :
            : "r" (active->sp)
        );
    }

    return 0;
}
