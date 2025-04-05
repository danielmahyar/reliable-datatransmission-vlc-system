#pragma once
#include <stdio.h>

typedef enum
{
    INFORMATION,
    SUCCESS,
    FAILURE,
    WARNING,
} LogLevel;

typedef enum
{
    NOT_SYNCED,
    TRYING_TO_SYNC,
    SYNCED
} ReceiverState;

typedef enum
{
    NOT_SENDING,
    SENDING_SYNC,
    SENDING_MESSAGE
} SenderState;

void log(LogLevel status, char *message, ...);
