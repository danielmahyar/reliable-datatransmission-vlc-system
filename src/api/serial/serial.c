#include "serial.h"

void get_line_input(char buf[], size_t len)
{
    memset(buf, 0, len);
    fpurge(stdin); // clears any junk in stdin

    while (1)
    {
        vTaskDelay(SAMPLING_RATE / portTICK_PERIOD_MS);

        if (fgets(buf, len, stdin) != NULL)
        {
            // Remove newline character if present
            size_t length = strlen(buf);
            if (length > 0 && buf[length - 1] == '\n')
            {
                buf[length - 1] = '\0';
            }
            break;
        }
    }
}