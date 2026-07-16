typedef struct {
    char *username;
    char *platform;
    char *command;
} CommandData;

typedef struct {
    char *key;
    void (*value)(struct CommandData *data);
} ListenerEntry;

void registerListener(char *command, void (*listener)(CommandData *data));
void processCommand(CommandData *data);