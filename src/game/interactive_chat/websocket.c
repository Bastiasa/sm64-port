#ifndef IC_WEBSOCKETS_MANAGEMENT
#define IC_WEBSOCKETS_MANAGEMENT

#include <stddef.h>
#include <libwebsockets.h>
#include <cjson/cJSON.h>
#include <stdio.h>

#include "pthread.h"
#include "commands_listeners.h"

static struct lws_context *gContext = NULL;
static pthread_t gWsThread;
static volatile int gRunning = 1;

static void processPacket(char *data, size_t len) {

    printf("[IC.WEBSOCKET]: %zu BYTES\n", len);

    const cJSON *packet = cJSON_ParseWithLength(data, len);

    if (!packet)
    {
        printf("[IC.WEBSOCKET]: INVALID JSON RECEIVED FROM SERVER\n");
        return;
    }

    char *packetType = cJSON_GetObjectItem(packet, "type")->valuestring;

    if (strcmp(packetType, "command") == 0)
    {
        printf("[IC.WEBSOCKET]: PACKET IS COMMAND TYPE.\n");

        CommandData data = {
            .command = cJSON_GetObjectItem(packet, "command")->valuestring,
            .username = cJSON_GetObjectItem(packet, "username")->valuestring,
            .platform = cJSON_GetObjectItem(packet, "platform")->valuestring,
        };

        processCommand(&data);
        cJSON_Delete(packet);
    } else {
        printf("[IC.WEBSOCKET]: UNKNOWN COMMAND TYPE.\n");
    }
    
}

static int onWebsocketCallback(
    struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len)
{
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf("[IC.WEBSOCKET]: CONNECTED\n");
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE:
            printf("[IC.WEBSOCKET]: MESSAGE RECEIVED\n");
            processPacket(in, len);
            break;

        case LWS_CALLBACK_CLIENT_CLOSED:
            printf("[IC.WEBSOCKET]: DISCONNECTED\n");
            break;

        default:
            break;
    }

    return 0;
}

static struct lws_protocols protocols[] = {
    {
        "chat-protocol",
        onWebsocketCallback,
        0,
        4096,
    },
    { NULL, NULL, 0, 0 }
};

static void *websocketThread(void *arg)
{
    while (gRunning) {
        lws_service(gContext, 50);
    }

    return NULL;
}

void websocketConnect(void)
{
    struct lws_client_connect_info ccinfo;
    memset(&ccinfo, 0, sizeof(ccinfo));

    ccinfo.context = gContext;
    ccinfo.address = "127.0.0.1";
    ccinfo.port = 3000;
    ccinfo.path = "/";
    ccinfo.host = ccinfo.address;
    ccinfo.origin = ccinfo.address;
    ccinfo.protocol = protocols[0].name;

    lws_client_connect_via_info(&ccinfo);
}

void startWebsocketThread(void)
{
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));

    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;

    gContext = lws_create_context(&info);

    if (!gContext) {
        printf("Failed to create context\n");
        return;
    }

    websocketConnect();

    pthread_create(
        &gWsThread,
        NULL,
        websocketThread,
        NULL
    );
}

#endif