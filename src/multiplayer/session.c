#include "global.h"
#include "multiplayer/dispatch.h"
#include "multiplayer/session.h"
#include "multiplayer/transport.h"

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

    MultiplayerDispatch_Tick(session);
}
