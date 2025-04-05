#include "serial.h"

uint8_t get_line_input(char buf[], size_t len, uint8_t* stop)
{
    memset(buf, 0, len);
    fpurge(stdin); // clears any junk in stdin

    while ((*stop) == 0)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        if (fgets(buf, len, stdin) != NULL)
        {
            // Remove newline character if present
            size_t length = strlen(buf);
            if (length > 0 && buf[length - 1] == '\n')
            {
                buf[length - 1] = '\0';
            }
            return 1;
        }
    }
    return 0;
}