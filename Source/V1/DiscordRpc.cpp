#include "DiscordRpc.h"
#include "DiscordRPCSDK/Includes/discord_rpc.h"

void Discord::Initialize()
{
    DiscordEventHandlers Handle;
    memset(&Handle, 0, sizeof(Handle));
    Discord_Initialize("901221838099742730", &Handle, 1, NULL);
}

void Discord::Update()
{
    DiscordRichPresence discord;
    memset(&discord, 0, sizeof(discord));
    discord.details = "Playing With GameSense";
    discord.state = "GameSense";
    discord.largeImageKey = "icon";
    discord.smallImageKey = "small icon";
    Discord_UpdatePresence(&discord);
}