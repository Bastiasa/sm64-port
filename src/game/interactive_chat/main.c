#ifndef INTERACTIVE_CHAT_MAIN
#define INTERACTIVE_CHAT_MAIN

#include <stdio.h>
#include <string.h>

#include "engine/math_util.h"

#include "websocket.h"
#include "commands_listeners.h"

#include "../object_helpers.h"
#include "../object_list_processor.h"
#include "../mario.h"
#include "../level_update.h"
#include "../spawn_object.h"        // para mark_obj_for_deletion
#include "object_fields.h"
#include "object_constants.h"
#include "model_ids.h"
#include "behavior_data.h"
#include  "sm64.h"
#include "types.h"
#include "PR/gbi.h"

#include "game/print.h"
#include "game/ingame_menu.h"
#include "game/game_init.h"
#include "game/segment2.h"


static struct Object *pBowserToUpdate = NULL;
static struct Object *nBowserToUpdate = NULL;

static void getMario_Forward(Vec3f out, f32 mult) {
    f32 forwardX = sins(gMarioState->faceAngle[1]);
    f32 forwardZ = coss(gMarioState->faceAngle[1]);

    out[0] = forwardX * mult;
    out[1] = 0.0f;
    out[2] = forwardZ * mult;
}

#define MESSAGE_DURATION 30 * 5

static char currentMessage[1024] = "";
static int messageTime = MESSAGE_DURATION + 1;

static u8 dialogBuffer[1024];

#define HIDDEN_TEXT_Y_POSITION -30
#define VISIBLE_TEXT_Y_POSITION 20

#define TEXT_FRAME_1_X_POSITION 30
#define TEXT_FRAME_2_X_POSITION 14

static s16 textY = HIDDEN_TEXT_Y_POSITION;
static s16 textYTarget = HIDDEN_TEXT_Y_POSITION;

static s16 textX = TEXT_FRAME_2_X_POSITION;
static s16 textXTarget = TEXT_FRAME_2_X_POSITION;

void pushCommandMessage(char *text, char *username) {
    snprintf(
        currentMessage, 
        sizeof(currentMessage), 
        text, 
        username ? username : "Desconocido"
    );
    messageTime = 0;
    textXTarget = TEXT_FRAME_1_X_POSITION;
    textX = TEXT_FRAME_2_X_POSITION;
}

void onExplosion(CommandData *data) {
    struct Object *explosion = spawn_object(
        gMarioObject,
        MODEL_EXPLOSION,
        bhvExplosion
    );

    explosion->oPosX = gMarioState->pos[0];
    explosion->oPosY = gMarioState->pos[1] + 30.0f;
    explosion->oPosZ = gMarioState->pos[2];

    printf("EXPLOSION SPAWNED!\n");
    pushCommandMessage("%s te ha hecho BOOM en la cara", data->username);
    
}

void onBowser(CommandData *data){

    Vec3f bowserPosition = {0.0f,0.0f,0.0f};
    getMario_Forward(bowserPosition, 600.0f);

    bowserPosition[0] += gMarioState->pos[0];
    bowserPosition[1] += gMarioState->pos[1];
    bowserPosition[2] += gMarioState->pos[2];


    struct Object *bowser = spawn_object_abs_with_rot(
        gMarioObject,
        0,
        MODEL_IC_BOWSER,
        bhvBowser,
        bowserPosition[0],
        bowserPosition[1],
        bowserPosition[2],
        0,
        0,
        0
    );

    pBowserToUpdate = bowser;

    pushCommandMessage(
        "%s te ha enviado un Bowser de regalito (%s)", 
        data->username
    );

    printf("BOWSER SPAWNED\n");
} 

void onCoin(CommandData *data) {
    struct Object *coin = spawn_object(
        gMarioObject,
        MODEL_YELLOW_COIN,
        bhvYellowCoin
    );

    coin->oPosX = gMarioObject->oPosX;
    coin->oPosY = gMarioObject->oPosY + 200.0f;
    coin->oPosZ = gMarioObject->oPosZ;

    pushCommandMessage("%s te ha dado una moneda", data->username);
}

void on1Up(CommandData *data) {
    struct Object *coin = spawn_object(
        gMarioObject,
        MODEL_1UP,
        bhv1Up
    );

    coin->oPosX = gMarioState->pos[0];
    coin->oPosY = gMarioState->pos[1];
    coin->oPosY = gMarioState->pos[2];
}

void onJump(CommandData *data) {
    set_mario_action(gMarioState, ACT_JUMP, 0);
    gMarioState->vel[1] = 150.0f;
    messageTime = 0;
    pushCommandMessage("%s te ha hecho saltar", data->username);
}

void onPunch(CommandData *data) {
    set_mario_action(gMarioState, ACT_PUNCHING, 0);
    pushCommandMessage("%s te ha hecho pegar", data->username);
}

void onBonk(CommandData *data) {
    set_mario_action(gMarioState, ACT_GROUND_BONK, 0);
    gMarioState->forwardVel = -450.0f;
    pushCommandMessage("%s hizo un muro invisible", data->username);
}

void onMiniBonk(CommandData *data) {
    set_mario_action(gMarioState, ACT_SOFT_BONK, 0);
    gMarioState->forwardVel = -50.0f;
    pushCommandMessage("%s te ha hecho chocarte", data->username);
}

void onBurn(CommandData *data) {
    /*struct Object *fire = spawn_object_abs_with_rot(
        gMarioObject,
        0,
        MODEL_RED_FLAME,
        bhvFlameMovingForwardGrowing,
        gMarioObject->oPosX,
        gMarioObject->oPosY + 100.0f,
        gMarioObject->oPosZ,
        0,
        0,
        0
    );*/

    hurt_and_set_mario_action(
        gMarioState,
        ACT_BURNING_GROUND,
        0,
        4 << 8
    );

    pushCommandMessage("%s te ha quemado el culo", data->username);
}

void ascii_to_dialog_string(const char *src) {
    int i = 0;

    while (src[i] && i < sizeof(dialogBuffer) - 1) {
        dialogBuffer[i] = ASCII_TO_DIALOG(src[i]);
        i++;
    }

    dialogBuffer[i] = DIALOG_CHAR_TERMINATOR;
}


f32 lerp(f32 value, f32 target, f32 weigth) {
    return value + (target - value) * weigth;
}

void renderTextBackground() {

    gDPSetCycleType(gDisplayListHead++, G_CYC_1CYCLE);
    gDPSetCombineMode(
        gDisplayListHead++, 
        G_CC_PRIMITIVE, 
        G_CC_PRIMITIVE
    );

    gDPSetRenderMode(
        gDisplayListHead++,
        G_RM_XLU_SURF,
        G_RM_XLU_SURF
    );

    gDPSetPrimColor(
        gDisplayListHead++,
        0,
        0,
        0,
        0,
        0,
        120
    );

    s16 sizeX = SCREEN_WIDTH;
    s16 sizeY = 20;

    s16 positionX = 0;
    s16 positionY = SCREEN_HEIGHT - textY - sizeY;

    gSPTextureRectangle(
        gDisplayListHead++,
        (int) (positionX) << 2,
        (int) (positionY) << 2,
        (int) (positionX + sizeX) << 2,
        (int) (positionY + sizeY) << 2,
        G_TX_RENDERTILE,
        0,
        0,
        0,
        0
    );

}

static void renderText() {
    gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);

    ascii_to_dialog_string(currentMessage);

    print_generic_string(
        textX, 
        textY, 
        dialogBuffer
    );

    gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
}


void interactiveChatOnRenderHud() {

    if (gMarioObject == NULL)
    {
        return;
    }

    textY = lerp(
        textY,
        textYTarget,
        0.1f
    );

    textX = lerp(
        textX,
        textXTarget,
        0.1f
    );
    

    renderTextBackground();
    renderText();

    if (messageTime >= MESSAGE_DURATION)
    {
        textYTarget = HIDDEN_TEXT_Y_POSITION;
    } else {
        textYTarget = VISIBLE_TEXT_Y_POSITION;
        messageTime++;
    }
    
}

void interactiveChatOnFrame() {

    if (nBowserToUpdate != NULL)
    {
        nBowserToUpdate->oAction = BOWSER_ACT_WALK_TO_MARIO;

        struct Object *bowser = nBowserToUpdate;

        bowser->oFaceAngleYaw = bowser->oAngleToMario;
        bowser->oAction = BOWSER_ACT_WALK_TO_MARIO;

        nBowserToUpdate = NULL;
    }
    

    if (pBowserToUpdate != NULL)
    {
        nBowserToUpdate = pBowserToUpdate;
        pBowserToUpdate = NULL;
    }
    
}

static int interactiveChatRunning = 0;

void runInteractiveChat(void) {

    if (interactiveChatRunning)
    {
        return;
    }

    interactiveChatRunning = 1;

    startWebsocketThread();

    FILE *f = fopen("logs.txt", "w");

    freopen("log.txt", "w", stdout);
    freopen("log.txt", "w", stderr);

    registerListener("bexplosion", onExplosion);
    registerListener("bbowser", onBowser);
    registerListener("bmoneda", onCoin);
    registerListener("b1up", on1Up);

    registerListener("bsaltar", onJump);
    registerListener("bpegar", onPunch);
    registerListener("bbonk", onBonk);
    registerListener("bminibonk", onMiniBonk);
    registerListener("bquemar", onBurn);
}

#endif