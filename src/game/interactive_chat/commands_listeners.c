
#include "stb_ds.h"
#include "commands_listeners.h"

#include <stdio.h>

static ListenerEntry *listeners = NULL;


void registerListener(char *command, void (*listener)(CommandData *data)) {
    printf("[IC.COMMANDS_LISTENER]: REGISTERING COMMAND %s\n", command);
    shput(listeners, command, listener);
}

void processCommand(CommandData *data) {

    int index = shgeti(listeners, data->command);

    printf("[IC.COMMANDS_LISTENER]: PROCESSING COMMAND %s\n", data->command);

    if (index == -1)
    {
        printf("[IC.COMMANDS_LISTENER]: COMMAND IS NOT REGISTERED.\n");
        return;
    }

    printf("[IC.COMMANDS_LISTENER]: COMMAND IS REGISTERED.");
    void (*listener)(CommandData *data) = shget(listeners, data->command);
    listener(data);
}