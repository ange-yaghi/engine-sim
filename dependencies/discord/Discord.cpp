#include "Discord.h"
#include <stdio.h>
#include <time.h>


const char* DISCORD_APPLICATION_ID = "1008609936256811038"; // https://discordapp.com/developers/applications

CDiscord* CDiscord::m_pInstance = NULL;
static int64_t StartTime;


CDiscord::CDiscord()
{
	m_state = "Doing Things";
	m_bEnableDiscord = true;
	m_bConnected = false;
}

CDiscord::~CDiscord()
{
}

bool CDiscord::CreateInstance()
{
	if (m_pInstance)
		return false;

	m_pInstance = new CDiscord;

	return true;
}

void CDiscord::DestroyInstance()
{
	if (!m_pInstance)
		return;

	Discord_Shutdown(); // shut down discord

	delete m_pInstance;
}

void CDiscord::handleDiscordReady(const DiscordUser * connectedUser)
{
	GetDiscordManager()->SetConnected(true);
}

void CDiscord::handleDiscordDisconnected(int errcode, const char* message)
{
	GetDiscordManager()->SetConnected(false);
}

void CDiscord::handleDiscordError(int errcode, const char* message)
{
}



void CDiscord::InitDiscord()
{
	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));

	handlers.ready = CDiscord::handleDiscordReady;
	handlers.disconnected = CDiscord::handleDiscordDisconnected;
	handlers.errored = CDiscord::handleDiscordError;
	handlers.joinGame = NULL;
	handlers.spectateGame = NULL;
	handlers.joinRequest = NULL;
	Discord_Initialize(DISCORD_APPLICATION_ID, &handlers, 1, NULL);
}

void CDiscord::UpdatePresence()
{
	if (m_bEnableDiscord == false)
		return;

	if (m_bConnected == false)
		return;

		DiscordRichPresence discordPresence;
		memset(&discordPresence, 0, sizeof(discordPresence));

		discordPresence.details = m_state.c_str();

		discordPresence.largeImageKey = "atgenginesim";
		discordPresence.largeImageText = "Engine Simulator";
		

		discordPresence.startTimestamp = StartTime;

		discordPresence.instance = 0;
		Discord_UpdatePresence(&discordPresence);
}


void CDiscord::ClearPresence()
{
	Discord_ClearPresence();
}


void CDiscord::UpdateDiscordConnection()
{
	if (m_bEnableDiscord == false)
		return;

#ifdef DISCORD_DISABLE_IO_THREAD
	Discord_UpdateConnection();
#endif
	Discord_RunCallbacks();
}

void CDiscord::SetUseDiscord(bool bFlag)
{
	m_bEnableDiscord = bFlag;

	if (bFlag)
	{
		InitDiscord();
	}
}


void CDiscord::SetConnected(bool bFlag)
{
	m_bConnected = bFlag;
}

void CDiscord::SetStatus(DiscordRichPresence presence, std::string engineName = "", std::string buildVersion = "")
{
	std::string engineString = "Engine: " + engineName;
	std::string stateString = "Build: " + buildVersion;
	presence.state = stateString.c_str();
	presence.details = engineString.c_str();
	presence.largeImageKey = "atgenginesim";
	presence.largeImageText = "Engine Simulator";
	StartTime = time(NULL);
	presence.instance = 1;
	presence.startTimestamp = StartTime;
	Discord_UpdatePresence(&presence);
}
