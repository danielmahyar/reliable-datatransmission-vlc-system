# Method 1 - Synchronous Emitter

## Overview of method

This implementation demonstrates a Visible Light Communication (VLC) emitter using FreeRTOS on a microcontroller. The emitter transmits data from one node to another by modulating an LED based on Manchester encoding. The approach is synchronous: data emission timing is controlled using `vTaskDelay`, ensuring precise intervals between bit transmissions. The system uses two FreeRTOS tasks pinned to separate cores—one for emitting data and another for reading incoming UART messages to be sent.

## FreeRTOS specific APIs used in the project

- `xTaskCreatePinnedToCore`: Creates and pins tasks to specific CPU cores for parallel execution.
- `vTaskDelay`: Suspends task execution for a specified period, used to control emission timing.
- `xQueueCreate`, `xQueueSend`, `xQueueReceive`: Implements a queue for inter-task communication, buffering encoded data bytes.
- `esp_task_wdt_deinit`: Disables the watchdog timer for uninterrupted task execution.

## Limitations

- The synchronous approach relies on accurate timing from `vTaskDelay`, which may be affected by task scheduling or system load.
- The queue-based design is not memory efficient, as each Manchester-encoded bit is sent individually.
- The implementation assumes reliable UART communication and does not handle transmission errors or synchronization loss.



