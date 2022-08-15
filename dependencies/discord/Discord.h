/*****************************************************************************
*
* File			: Discord.h
* Author		: SanGawku
* Copyright		: SanGawku
* Date			: 7/29/2019
* Abstract		: Discord
*****************************************************************************
*
* Desc         : Discord rich presence integration
*
*****************************************************************************/
#ifndef __DISCORD_H__
#define __DISCORD_H__

#include <string>
#include "discord_rpc.h"
#pragma comment(lib, "discord-rpc.lib")

class CDiscord
{

public:

	CDiscord();
	virtual ~CDiscord();

	static bool		CreateInstance();
	static void		DestroyInstance();

	static CDiscord* GetInstance() { return m_pInstance; }

public:

	static void handleDiscordReady(const DiscordUser* connectedUser);

	static void handleDiscordDisconnected(int errcode, const char* message);

	static void handleDiscordError(int errcode, const char* message);

private:

	void InitDiscord();

	void UpdatePresence();

	void ClearPresence();

public:

	void			UpdateDiscordConnection();

	void			SetUseDiscord(bool bFlag); // if true, then we will use discord and connect to discord

	void			SetConnected(bool bFlag); //set connected to discord or not

	void			SetStatus(DiscordRichPresence presence, std::string engineName, std::string buildVersion);



private:

	std::string		m_state;
	
	std::string		m_engineName;

	bool			m_bEnableDiscord;

	bool			m_bConnected;

protected:

	static CDiscord* m_pInstance;
};

static CDiscord* GetDiscordManager()
{
	return CDiscord::GetInstance();
};


#endif