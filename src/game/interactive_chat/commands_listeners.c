
#include "stb_ds.h"
#include "commands_listeners.h"

static ListenerEntry *listeners = NULL;


void registerListener(char *command, void (*listener)(CommandData *data)) {
    hmput(listeners, command, listener);
}

void processCommand(CommandData *data) {

    int index = shgeti(listeners, data->command);

    if (index < 0)
    {
        return;
    }

    ListenerEntry *listenerEntry = shget(listeners, data->command);
    listenerEntry->value(data);
}