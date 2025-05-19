# Method 2 - Async Receiver

## Overview of method

This implementation demonstrates a Visible Light Communication (VLC) receiver using FreeRTOS on a microcontroller, employing an interrupt-driven (ISR) and asynchronous approach. The receiver samples the incoming light signal using a hardware timer interrupt, enabling precise and immediate detection of Manchester-encoded data sent by the emitter.

To improve robustness against noise and timing errors, the receiver uses oversampling: each bit is sampled multiple times, and a stable value is determined before processing. A Deterministic Finite Automaton (DFA) is used to synchronize to the start of valid Manchester-encoded frames and to validate the structure of received data. This ensures that only correctly formatted data is accepted and decoded. Once a valid byte is reconstructed, it is sent to a FreeRTOS queue and forwarded to UART for further processing.

## FreeRTOS specific APIs used in the project

- `xTaskCreatePinnedToCore`: Creates and pins tasks to specific CPU cores for parallel execution.
- `xQueueCreate`, `xQueueSendFromISR`, `xQueueReceive`: Implements a queue for ISR-to-task communication, buffering decoded bytes between the ISR and UART tasks.
- `esp_timer_create`, `esp_timer_start_periodic`: Sets up a periodic hardware timer to trigger the sampling ISR at precise intervals.
- `esp_task_wdt_deinit`: Disables the watchdog timer for uninterrupted task execution.

## Limitations

- The implementation relies on correct ISR and timer configuration; missed interrupts or timing drift may affect data integrity.
- Oversampling and DFA logic increase complexity and may introduce latency if not tuned properly.
- The system assumes reliable UART communication and does not handle transmission errors or loss of synchronization.
- High CPU load or long ISR execution times can impact the accuracy of bit sampling and overall system responsiveness.

