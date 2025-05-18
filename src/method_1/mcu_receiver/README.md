# Method 1 - Synchronous Receiver

## Overview of method

This implementation demonstrates a Visible Light Communication (VLC) receiver using FreeRTOS on a microcontroller. The receiver synchronizes to an incoming VLC signal, decodes Manchester-encoded data, and relays the received bytes over UART. The approach is synchronous: the receiver samples the light input at fixed intervals using `vTaskDelay`, attempting to maintain alignment with the emitter. Two FreeRTOS tasks are used: one for sampling and decoding the VLC signal, and another for forwarding decoded bytes to UART.

## FreeRTOS specific APIs used in the project

- `xTaskCreatePinnedToCore`: Creates and pins tasks to specific CPU cores for parallel execution.
- `vTaskDelay`: Suspends task execution for a specified period, used to control sampling intervals.
- `xQueueCreate`, `xQueueSendFromISR`, `xQueueReceive`: Implements a queue for inter-task communication, buffering decoded bytes between the receiver and UART tasks.
- `esp_task_wdt_deinit`: Disables the watchdog timer for uninterrupted task execution.

## Limitations

- The synchronous approach depends on accurate timing from `vTaskDelay`, which may be affected by task scheduling or system load.
- Synchronization to the emitter can be lost if timing drifts or if there is significant noise in the received signal.
- The implementation assumes reliable UART communication and does not handle transmission errors or loss of synchronization.

