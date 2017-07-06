#include <interrupt.h>
#include <user/bwio.h>
#include <panic.h>
#include <task.h>
#include <user/ts7200.h>
#include <user/err.h>
#include <user/std.h>

extern "C" void irqEntry();

namespace kernel {

void InterruptController::clearAll() {
    // Disable all interrupts
    *(volatile unsigned*)(VIC1Base + VICxIntEnClear) = 0xffffffff;
    *(volatile unsigned*)(VIC2Base + VICxIntEnClear) = 0xffffffff;
    // Set all interrupts to IRQ
    *(volatile unsigned*)(VIC1Base + VICxIntSelect) = 0;
    *(volatile unsigned*)(VIC2Base + VICxIntSelect) = 0;
}

int InterruptController::awaken(Scheduler &scheduler, const ctl::Event event, int ret) {
    const int eventId = static_cast<int>(event);

    // Iterate over and awaken all tasks blocked on the event.
    Td *td;
    int count = 0;
    while ((td = awaitQueues[eventId])) {
        awaitQueues[eventId] = td->nextReady; // Pop.
        td->setReturn(ret);
        scheduler.readyTask(*td);
        ++count;
    }
    return count;
}

InterruptController::InterruptController() {
    clearAll();
    *(volatile unsigned*)(0x38) = (unsigned)irqEntry;
    *(volatile unsigned*)(VIC1Base + VICxIntEnable) = (1u << TC1UI);
    *(volatile unsigned*)(VIC2Base + VICxIntEnable) = (1u << INT_UART1) + (1u << INT_UART2);
}

InterruptController::~InterruptController() {
    clearAll();
}

int InterruptController::awaitEvent(Td *td, const ctl::Event event) {
    // Check for invalid id.
    // TODO: I don't know if it's necessary to check if enum < 0
    const int eventId = static_cast<int>(event);
    if (eventId < 0 || ctl::EVENT_NUM <= eventId) {
        return -static_cast<int>(ctl::Error::InvId);
    }

    // Enable the interrupt if necessary.
    switch (event) {
        case ctl::Event::Uart1Tx: {
            *(volatile unsigned *)(UART1_BASE + UART_CTLR_OFFSET) |= TIEN_MASK;
            break;
        }
        case ctl::Event::Uart2Tx: {
            *(volatile unsigned *)(UART2_BASE + UART_CTLR_OFFSET) |= TIEN_MASK;
            break;
        }
        default: {
            break;
        }
    }

    // Update td state and insert into await queue.
    td->state = RunState::EventBlocked;
    td->nextReady = awaitQueues[eventId];
    awaitQueues[eventId] = td;

    return 0;
}

void InterruptController::handle(Scheduler &scheduler) {
    const auto vic1Status = *(volatile unsigned*)(VIC1Base + VICxIRQStatus);
    const auto vic2Status = *(volatile unsigned*)(VIC2Base + VICxIRQStatus);

    // Clear interrupt and awaken tasks for each interrupt type.
    if (vic1Status & (1u << TC1UI)) {
        *(volatile unsigned *)(TIMER1_BASE + CLR_OFFSET) = 0;
        awaken(scheduler, ctl::Event::PeriodicTimer, 0);
    }
    if (vic2Status & (1u << INT_UART1)) {
        const auto uartStatus = *(volatile unsigned*)(UART1_BASE + UART_INTR_OFFSET);
        if (uartStatus & TIS_MASK) {
            *(volatile unsigned*)(UART1_BASE + UART_CTLR_OFFSET) &= ~TIEN_MASK;

            if (ctsHigh && ctsLow && awaken(scheduler, ctl::Event::Uart1Tx, 0)) {
                ctsHigh = ctsLow = false;
            }
        }
        if (uartStatus & MIS_MASK) {
            if (*(volatile unsigned*)(UART1_BASE + UART_MDMSTS_OFFSET) & 0x1) {
                if (*(volatile unsigned*)(UART1_BASE + UART_FLAG_OFFSET) & CTS_MASK) {
                    ctsHigh = true;
                }
                else {
                    ctsLow = true;
                }
                if (ctsHigh && ctsLow && awaken(scheduler, ctl::Event::Uart1Tx, 0)) {
                    ctsHigh = ctsLow = false;
                }
            }
            *(volatile unsigned*)(UART1_BASE + UART_INTR_OFFSET) = 0;
        }
        if (uartStatus & RIS_MASK) {
            int ret = *(volatile unsigned*)(UART1_BASE + UART_DATA_OFFSET) & 0xff;
            // TODO: corrupt data
            awaken(scheduler, ctl::Event::Uart1Rx, ret);
        }
    }
    if (vic2Status & (1u << INT_UART2)) {
        const auto uartStatus = *(volatile unsigned*)(UART2_BASE + UART_INTR_OFFSET);
        if (uartStatus & TIS_MASK) {
            *(volatile unsigned*)(UART2_BASE + UART_CTLR_OFFSET) &= ~TIEN_MASK;
            awaken(scheduler, ctl::Event::Uart2Tx, 0);
        }
        if (uartStatus & RIS_MASK) {
            int ret = *(volatile unsigned*)(UART2_BASE + UART_DATA_OFFSET) & 0xff;
            // TODO: corrupt data
            awaken(scheduler, ctl::Event::Uart2Rx, ret);
        }
    }
}
} // namespace kernel
