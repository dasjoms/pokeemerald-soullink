#include "global.h"
#include "link.h"
#include "multiplayer/dispatch.h"
#include "multiplayer/session.h"
#include "multiplayer/transport.h"

static EWRAM_DATA bool8 sLinkProbeStarted = FALSE;
static EWRAM_DATA enum MultiplayerLinkState sLinkState = MULTIPLAYER_LINK_OFFLINE;
static EWRAM_DATA u8 sPlayerCount = 0;
static EWRAM_DATA u16 sProbeFrames = 0;

static void UpdateLinkProbeState(void)
{
    sPlayerCount = GetLinkPlayerCount();

    if (HasLinkErrorOccurred())
    {
        sLinkState = MULTIPLAYER_LINK_ERROR;
    }
    else if (IsLinkConnectionEstablished() && sPlayerCount > 1)
    {
        sLinkState = MULTIPLAYER_LINK_ONLINE;
    }
    else if (sLinkProbeStarted && sProbeFrames < 180)
    {
        sLinkState = MULTIPLAYER_LINK_CONNECTING;
    }
    else
    {
        sLinkState = MULTIPLAYER_LINK_OFFLINE;
    }

    if (sLinkProbeStarted && sLinkState != MULTIPLAYER_LINK_ONLINE)
        sProbeFrames++;
}

void MultiplayerSession_Init(struct MultiplayerSession *session)
{
    session->active = FALSE;
    session->localPlayerId = 0;
    session->playerCount = 0;

    MultiplayerTransportLink_Init();
    MultiplayerDispatch_Init();
}

void MultiplayerSession_Start(struct MultiplayerSession *session)
{
    session->active = TRUE;
}

void MultiplayerSession_Stop(struct MultiplayerSession *session)
{
    session->active = FALSE;
}

void MultiplayerSession_Tick(struct MultiplayerSession *session)
{
    if (!session->active)
        return;

    UpdateLinkProbeState();
    MultiplayerDispatch_Tick(session);
}

void MultiplayerSession_StartLinkProbe(void)
{
    if (!sLinkProbeStarted)
    {
        gLinkType = LINKTYPE_BATTLE;
        OpenLink();
        sLinkProbeStarted = TRUE;
        sProbeFrames = 0;
    }

    UpdateLinkProbeState();
}

void MultiplayerSession_StopLinkProbe(void)
{
    if (sLinkProbeStarted)
    {
        CloseLink();
        sLinkProbeStarted = FALSE;
    }

    sProbeFrames = 0;

    UpdateLinkProbeState();
}

enum MultiplayerLinkState MultiplayerSession_GetLinkState(void)
{
    UpdateLinkProbeState();
    return sLinkState;
}

u8 MultiplayerSession_GetPlayerCount(void)
{
    UpdateLinkProbeState();
    return sPlayerCount;
}
