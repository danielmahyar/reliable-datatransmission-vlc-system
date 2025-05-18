# Method 2 - Asynchronous Emitter

## Overview of method

This implementation demonstrates a Visible Light Communication (VLC) emitter using FreeRTOS on a microcontroller, utilizing an interrupt-driven (ISR) and asynchronous approach. The emitter transmits data by modulating an LED with Manchester-encoded bits, where timing is controlled by a hardware timer interrupt (ISR) rather than task delays. Data to be sent is queued and emitted bit-by-bit in the ISR, allowing for precise and non-blocking transmission. UART is used to receive messages to be sent, which are then encoded and queued for emission.

## FreeRTOS specific APIs used in the project

- `xTaskCreatePinnedToCore`: Creates and pins tasks to specific CPU cores for parallel execution.
- `xQueueCreate`, `xQueueSend`, `xQueueReceiveFromISR`: Implements a queue for inter-task and ISR-to-task communication, buffering encoded data bits.
- `esp_timer_create`, `esp_timer_start_periodic`: Sets up a periodic hardware timer to trigger the emission ISR at precise intervals.
- `esp_task_wdt_deinit`: Disables the watchdog timer for uninterrupted task execution.

## Limitations

- The implementation relies on correct ISR and timer configuration; timing issues or missed interrupts may affect data integrity.
- The queue-based design is not memory efficient, as each Manchester-encoded bit is sent individually.
- The system assumes reliable UART communication and does not handle transmission errors or synchronization loss.

