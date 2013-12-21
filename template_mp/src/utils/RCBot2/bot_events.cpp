/*
 *    This file is part of RCBot.
 *
 *    RCBot by Paul Murphy adapted from Botman's HPB Bot 2 template.
 *
 *    RCBot is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    RCBot is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with RCBot; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */
#include "igameevents.h"
#include "bot.h"
#include "bot_event.h"
#include "bot_strings.h"
#include "bot_globals.h"
#include "bot_fortress.h"
#include "bot_script.h"
#include "bot_dod_bot.h"
#include "bot_weapons.h"
#include "bot_getprop.h"
#include "bot_dod_bot.h"

vector<CBotEvent*> CBotEvents :: m_theEvents;
extern ConVar bot_use_vc_commands;
///////////////////////////////////////////////////////

class CBotSeeFriendlyKill : public IBotFunction
{
public:
	CBotSeeFriendlyKill ( edict_t *pTeammate, edict_t *pDied, const char *szKillerWeapon )
	{
		m_pTeammate = pTeammate;
		m_pWeapon = CWeapons::getWeaponByShortName(szKillerWeapon);
		m_pDied = pDied;
	}
	void execute ( CBot *pBot )
	{
		if ( CClassInterface::getTeam(m_pTeammate) != pBot->getTeam() )
			return;

		if ( pBot->getEdict() != m_pTeammate )
		{
			if ( pBot->isVisible(m_pTeammate) )
				pBot->seeFriendlyKill(m_pTeammate,m_pDied,m_pWeapon);
		}
	}
private:
	edict_t *m_pTeammate;
	edict_t *m_pDied;
	CWeapon *m_pWeapon;
};

class CBotSeeFriendlyHurtEnemy : public IBotFunction
{
public:
	CBotSeeFriendlyHurtEnemy ( edict_t *pTeammate, edict_t *pEnemy )
	{
		m_pTeammate = pTeammate;
		m_pEnemy = pEnemy;
	}

	void execute ( CBot *pBot )
	{
		if ( CClassInterface::getTeam(m_pTeammate) != pBot->getTeam() )
			return;

		if ( pBot->getEdict() != m_pTeammate )
		{
			if ( pBot->isVisible(m_pTeammate) && pBot->isVisible(m_pEnemy) )
				pBot->seeFriendlyHurtEnemy(m_pTeammate,m_pEnemy,NULL);
		}
	}
private:
	edict_t *m_pTeammate;
	edict_t *m_pEnemy;
};

class CBotSeeFriendlyDie : public IBotFunction
{
public:
	CBotSeeFriendlyDie ( edict_t *pDied, edict_t *pKiller, const char *szKillerWeapon )
	{
		m_pDied = pDied;
		m_pWeapon = CWeapons::getWeaponByShortName(szKillerWeapon);
		m_pKiller = pKiller;
	}
	void execute ( CBot *pBot )
	{
		if ( CClassInterface::getTeam(m_pDied) != pBot->getTeam() )
			return;

		if ( pBot->getEdict() != m_pDied )
		{
			if ( pBot->isVisible(m_pDied) )
				pBot->seeFriendlyDie(m_pDied,m_pKiller,m_pWeapon);
		}
	}
private:
	edict_t *m_pDied;
	edict_t *m_pKiller;
	CWeapon *m_pWeapon;
};

////////////////////////////////////////////////


void CRoundStartEvent :: execute ( IBotEventInterface *pEvent )
{
	CBots::roundStart();
}

void CPlayerHurtEvent :: execute ( IBotEventInterface *pEvent )
{
	CBot *pBot = CBots::getBotPointer(m_pActivator);
	edict_t *pAttacker = CBotGlobals::playerByUserId(pEvent->getInt("attacker"));

	if ( m_pActivator != pAttacker )
	{
		if ( pAttacker && (!pAttacker->m_pNetworkable || !pAttacker->m_NetworkSerialNumber) )
			pAttacker = NULL;

		if ( pBot )
		{
			pBot->hurt(pAttacker,pEvent->getInt("health"));
		}

		pBot = CBots::getBotPointer(pAttacker);

		if ( pBot )
		{
			pBot->shot(m_pActivator);
		}

		if ( CBotGlobals::isPlayer(m_pActivator) && CBotGlobals::isPlayer(pAttacker) )
		{
			CBotSeeFriendlyHurtEnemy func1(pAttacker,m_pActivator);

			CBots::botFunction(&func1);
		}
	}
	//CBots::botFunction()
}

void CPlayerDeathEvent :: execute ( IBotEventInterface *pEvent )
{
	CBot *pBot = CBots::getBotPointer(m_pActivator);
	const char *weapon = pEvent->getString("weapon",NULL);

	edict_t *pAttacker = CBotGlobals::playerByUserId(pEvent->getInt("attacker"));
	
	if ( pAttacker && ((CBotGlobals::entityOrigin(pAttacker)-CBotGlobals::entityOrigin(m_pActivator)).Length()>512.0f) )
	{
		// killer
		CClient *pClient = CClients::get(pAttacker);

		if ( pClient && pClient->autoWaypointOn() )
		{
			CWeapon *pWeapon = CWeapons::getWeaponByShortName(weapon);

			if ( pWeapon != NULL )
			{
				if ( pWeapon->isScoped() )
				{
					pClient->autoEventWaypoint(CWaypointTypes::W_FL_SNIPER,100.0f);
				}
				else if ( pWeapon->isDeployable() )
				{
					// non OO hack here
					if ( CBotGlobals::isCurrentMod(MOD_DOD) )
					{
						edict_t *pentWeapon = CWeapons::findWeapon(pAttacker,pWeapon->getWeaponName());

						if ( CClassInterface::isMachineGunDeployed(pentWeapon) )
						{
							bool bIsProne;
							float flStamina;

							CClassInterface::getPlayerInfoDOD(pAttacker,&bIsProne,&flStamina);

							if ( !bIsProne )
							{
								pClient->autoEventWaypoint(CWaypointTypes::W_FL_MACHINEGUN,100.0f);
							}
						}
					}
					//CClassInterface::isMachineGunDeployed(pWeapon->get)
					//pWeapon->isDeployed()
				}
			}
		}

		// victim
		pClient = CClients::get(m_pActivator);

		if ( CBotGlobals::isPlayer(pAttacker) && pClient && pClient->autoWaypointOn() )
		{
			CWeapon *pWeapon = CWeapons::getWeaponByShortName(weapon);

			if ( pWeapon != NULL )
			{
				if ( pWeapon->isScoped() )
				{
					pClient->autoEventWaypoint(CWaypointTypes::W_FL_SNIPER,200.0f,true,CClassInterface::getTeam(pAttacker),CBotGlobals::entityOrigin(pAttacker)+Vector(0,0,32.0f));
				}
			}
		}

	}

	if ( pBot )
		pBot->died(pAttacker,weapon);

	pBot = CBots::getBotPointer(pAttacker);

	if ( pBot )
	{
		pBot->killed(m_pActivator,(char*)weapon);

		pBot->enemyDown(m_pActivator);
	}
	
	if ( m_pActivator && pAttacker ) // not worldspawn
	{
		CBotSeeFriendlyDie func1(m_pActivator,pAttacker,weapon);
		CBotSeeFriendlyKill func2(pAttacker,m_pActivator,weapon);

		CBots::botFunction(&func1);
		CBots::botFunction(&func2);
	}
}

void CBombPickupEvent :: execute ( IBotEventInterface *pEvent )
{
}

void CPlayerFootstepEvent :: execute ( IBotEventInterface *pEvent )
{
}

void CBombDroppedEvent :: execute ( IBotEventInterface *pEvent )
{
}

void CWeaponFireEvent :: execute ( IBotEventInterface *pEvent )
{
}

void CPlayerSpawnEvent :: execute ( IBotEventInterface *pEvent )
{
	CBot *pBot = CBots::getBotPointer(m_pActivator);

	if ( pBot )
		pBot->spawnInit();

	if ( CBotGlobals::isCurrentMod(MOD_TF2) )
	{
		if ( pEvent->getInt("class") == TF_CLASS_MEDIC )
		{
			// find medigun
			CTeamFortress2Mod::findMediGun(m_pActivator);
		}
	}
}

void CBulletImpactEvent :: execute ( IBotEventInterface *pEvent )
{
	CBot *pBot = CBots::getBotPointer(m_pActivator);

	if ( pBot )
	{
		pBot->shotmiss();
	}
}
/////////////////////////////////////////

/*
[RCBot] [DEBUG game_event] player_sapped_object
[RCBot] [DEBUG game_event] userid = 2
[RCBot] [DEBUG game_event] ownerid = 4
[RCBot] [DEBUG game_event] object = 2
[RCBot] [DEBUG game_event] sapperid = 400
*/
void CTF2ObjectSapped :: execute ( IBotEventInterface *pEvent )
{
	int owner = pEvent->getInt("ownerid",-1);
	int building = pEvent->getInt("object",-1);
	int sapperid = pEvent->getInt("sapperid",-1);

	if ( m_pActivator && (owner>=0) && (building>=0) && (sapperid>=0) )
	{
		edict_t *pSpy = m_pActivator;
		edict_t *pOwner = CBotGlobals::playerByUserId(owner);
		edict_t *pSapper = INDEXENT(sapperid);
		CBotTF2 *pBot = (CBotTF2*)CBots::getBotPointer(pOwner);
		
		if ( pBot )
		{
			pBot->buildingSapped((eEngiBuild)building,pSapper,pSpy);
		}

		CTeamFortress2Mod::sapperPlaced(pOwner,(eEngiBuild)building,pSapper);

		CBroadcastSpySap spysap = CBroadcastSpySap(pSpy);

		CBots::botFunction(&spysap);

	}
}

void CTF2RoundActive :: execute ( IBotEventInterface *pEvent )
{
	CTeamFortress2Mod::resetSetupTime();
}

void COverTimeBegin :: execute ( IBotEventInterface *pEvent )
{
	CBroadcastOvertime function;
	
	CBots::botFunction(&function);
}

void CBossSummonedEvent :: execute ( IBotEventInterface *pEvent )
{
	CTeamFortress2Mod::initBoss(true);
}

void CBossKilledEvent :: execute ( IBotEventInterface *pEvent )
{
	CTeamFortress2Mod::initBoss(false);
}

void CPlayerTeleported ::execute(IBotEventInterface *pEvent)
{
	int builderid = pEvent->getInt("builderid");

	CBot *pBot = CBots::getBotPointer(CBotGlobals::playerByUserId(builderid));

	if ( pBot )
	{
		((CBotTF2*)pBot)->teleportedPlayer();
	}
}

void CPlayerHealed ::execute(IBotEventInterface *pEvent)
{
	int patient = pEvent->getInt("patient",-1);
	int healer = pEvent->getInt("healer",-1);
	int amount = pEvent->getFloat("amount",0);

	if ( (healer != -1) && ( patient != -1 ) && (healer != patient) )
	{
		m_pActivator = CBotGlobals::playerByUserId(patient);

		if ( m_pActivator )
		{
			CBot *pBot = CBots::getBotPointer(m_pActivator);

			if ( pBot )
			{
				CBotTF2 *pBotTF2 = (CBotTF2*)pBot;

				if ( pBotTF2 && randomInt(0,1) )
					pBotTF2->addVoiceCommand(TF_VC_THANKS);
			}
		}

		CBot *pBot = CBots::getBotPointer(CBotGlobals::playerByUserId(healer));

		if ( pBot && pBot->isTF2() )
		{
			((CBotTF2*)pBot)->healedPlayer(m_pActivator,amount);
		}
	}
}

/*
[RCBot] [DEBUG game_event] object_destroyed
[RCBot] [DEBUG game_event] userid = 2
[RCBot] [DEBUG game_event] attacker = 4
[RCBot] [DEBUG game_event] weapon = wrench
[RCBot] [DEBUG game_event] weapon_logclassname = wrench
[RCBot] [DEBUG game_event] weaponid = 10
[RCBot] [DEBUG game_event] priority = 6
[RCBot] [DEBUG game_event] objecttype = 3
[RCBot] [DEBUG game_event] index = 436
[RCBot] [DEBUG game_event] was_building = 0
*/
void CTF2ObjectDestroyed :: execute ( IBotEventInterface *pEvent )
{
	int type = pEvent->getInt("objecttype",-1);
	int index = pEvent->getInt("index",-1);
	int was_building = pEvent->getInt("was_building",-1);
	edict_t *pAttacker = CBotGlobals::playerByUserId(pEvent->getInt("attacker",NULL));

	if ( pAttacker && m_pActivator && (type>=0) && (index>=0) && (was_building>=0) )
	{
		//if ( !was_building )
		//{ // could be a sapper
		if ( (eEngiBuild)type == ENGI_SAPPER )
		{
			edict_t *pOwner = pAttacker;
			edict_t *pSapper = INDEXENT(index);
			CBotTF2 *pBot = (CBotTF2*)CBots::getBotPointer(pOwner);

			if ( pBot )
				pBot->sapperDestroyed(pSapper);

			CTeamFortress2Mod::sapperDestroyed(pOwner,(eEngiBuild)type,pSapper);
		}
		else
		{
			CBotTF2 *pBot = (CBotTF2*)CBots::getBotPointer(m_pActivator);

			if ( pBot )
			{
				edict_t *pBuilding = INDEXENT(index);

				pBot->buildingDestroyed(type,pAttacker,pBuilding);
			}
		}
		//}
	}


}

/*
player_upgradedobject
Name: 	player_upgradedobject
Structure: 	
short 	userid 	
byte 	object 	
short 	index 	
bool 	isbuilder 	
*/
void CTF2UpgradeObjectEvent :: execute ( IBotEventInterface *pEvent )
{
	if ( bot_use_vc_commands.GetBool() && randomInt(0,1) )
	{
		eEngiBuild object = (eEngiBuild)pEvent->getInt("object",0);
		bool isbuilder = (pEvent->getInt("isbuilder")>0);
		short index = pEvent->getInt("index");
	
		if ( !isbuilder )
		{
			// see if builder is a bot
			edict_t *pOwner = CTeamFortress2Mod::getBuildingOwner (object, index);
			CBotTF2 *pBot;

			if ( (pBot = (CBotTF2*)CBots::getBotPointer(pOwner)) != NULL )
			{
				pBot->addVoiceCommand(TF_VC_THANKS);
			}
		}
	}
}

void CTF2SetupFinished ::execute(IBotEventInterface *pEvent )
{
	CTeamFortress2Mod::roundStarted();
}

void CTF2BuiltObjectEvent :: execute ( IBotEventInterface *pEvent )
{
	eEngiBuild type = (eEngiBuild)pEvent->getInt("object");
	int index = pEvent->getInt("index");
	edict_t *pBuilding = INDEXENT(index);
	CBot *pBot = CBots::getBotPointer(m_pActivator);

	CClient *pClient = CClients::get(m_pActivator);

	if ( type == ENGI_TELE )
	{
		CTeamFortress2Mod::teleporterBuilt(m_pActivator,type,pBuilding);

		if ( pClient && pClient->autoWaypointOn() )
		{
			if ( CTeamFortress2Mod::isTeleporterEntrance(pBuilding,CTeamFortress2Mod::getTeam(m_pActivator)) )
				pClient->autoEventWaypoint(CWaypointTypes::W_FL_TELE_ENTRANCE,400.0f);
			else
				pClient->autoEventWaypoint(CWaypointTypes::W_FL_TELE_EXIT,400.0f);
		}
	}

	if ( type == ENGI_SENTRY )
	{
		CTeamFortress2Mod::sentryBuilt(m_pActivator,type,pBuilding);

		if ( pClient && pClient->autoWaypointOn() )
		{
			pClient->autoEventWaypoint(CWaypointTypes::W_FL_SENTRY,400.0f);
		}
	}

	if ( type == ENGI_DISP )
	{
		CTeamFortress2Mod::dispenserBuilt(m_pActivator,type,pBuilding);
	}

	if ( pBot && pBot->isTF() )
	{
		((CBotFortress*)pBot)->engiBuildSuccess((eEngiBuild)pEvent->getInt("object"),pEvent->getInt("index"));
	}
}

void CTF2ChangeClass :: execute ( IBotEventInterface *pEvent )
{
	CBot *pBot = CBots::getBotPointer(m_pActivator);

	if ( pBot && pBot->isTF() )
	{

		int _class = pEvent->getInt("class");

		((CBotFortress*)pBot)->setClass((TF_Class)_class);

	}
}

void CTF2RoundStart :: execute ( IBotEventInterface *pEvent )
{
	// 04/07/09 : add full reset

	  CBroadcastRoundStart roundstart = CBroadcastRoundStart(pEvent->getInt("full_reset") == 1);
	  
	  if ( pEvent->getInt("full_reset") == 1 )
	  {
		CPoints::resetPoints();
	  }

	  // MUST BE AFTER RESETPOINTS
	  CTeamFortress2Mod::roundReset();
	  CBots::botFunction(&roundstart);
	  CTeamFortress2Mod::resetSetupTime();
}
/*
teamplay_capture_broken
Name: 	teamplay_capture_broken
Structure: 	
byte 	cp 	
string 	cpname 	
float 	time_remaining 
*/
void CTF2PointStopCapture :: execute ( IBotEventInterface *pEvent )
{
	int capindex = pEvent->getInt("cp",0);

	CTeamFortress2Mod::removeCappers(capindex);
	
}
/*
teamplay_capture_blocked

Note: When a player blocks the capture of a control point
Name: 	teamplay_capture_blocked
Structure: 	
byte 	cp 	index of the point that was blocked
string 	cpname 	name of the point
byte 	blocker 	index of the player that blocked the cap 
*/
void CTF2PointBlockedCapture :: execute ( IBotEventInterface *pEvent )
{
	int capindex = pEvent->getInt("cp",0);

	CTeamFortress2Mod::removeCappers(capindex);
}

void CTF2PointStartCapture :: execute ( IBotEventInterface *pEvent )
{/*
 [RCBot] [DEBUG game_event] teamplay_point_startcapture
[RCBot] [DEBUG game_event] cp = 0
[RCBot] [DEBUG game_event] cpname = #Dustbowl_cap_1_A
[RCBot] [DEBUG game_event] team = 2
[RCBot] [DEBUG game_event] capteam = 3
[RCBot] [DEBUG game_event] captime = 64.134995
[RCBot] [DEBUG game_event] cappers = 
[RCBot] [DEBUG game_event] priority = 7
*/
	int capteam = pEvent->getInt("capteam",0);
	int capindex = pEvent->getInt("cp",0);
	const char *cappers = pEvent->getString("cappers",NULL);
	const char *cpname = pEvent->getString("cpname");

	if ( cappers )
	{
		int i = 0;

		while ( cappers[i] != 0 )
		{
			CTeamFortress2Mod::addCapper(capindex,(int)cappers[i]);
			i++;
		}
	}

	CPoints::pointBeingCaptured(capteam,cpname);

	CBotTF2FunctionEnemyAtIntel *function = new CBotTF2FunctionEnemyAtIntel(capteam,Vector(0,0,0),EVENT_CAPPOINT);

	CBots::botFunction(function);

	delete function;
	
}

void CTF2PointCaptured :: execute ( IBotEventInterface *pEvent )
{
	CBroadcastCapturedPoint cap = CBroadcastCapturedPoint(pEvent->getInt("cp"),pEvent->getInt("team"),pEvent->getString("cpname"));
	
	CPoints::pointCaptured(pEvent->getInt("team"),pEvent->getString("cpname"));
    // MUST BE AFTER POINTCAPTURED
    CBots::botFunction(&cap);
}

/* Flag has been picked up or dropped */
#define FLAG_PICKUP		1
#define FLAG_CAPTURED	2
#define FLAG_DEFEND		3
#define FLAG_DROPPED	4
#define FLAG_RETURN		5

void CFlagEvent :: execute ( IBotEventInterface *pEvent )
{
	// dropped / picked up ID
	int type = pEvent->getInt("eventtype");
	// player id
	int player = pEvent->getInt("player");

	edict_t *pPlayer = NULL;
	CBot *pBot = NULL;
	
	// Crash fix
	if ( player )
	{
		pPlayer = INDEXENT(player);
		pBot = CBots::getBotPointer(pPlayer);
	}

	switch ( type )
	{
	case FLAG_PICKUP: // pickup
		if ( pBot && pBot->isTF() )
		{
			((CBotTF2*)pBot)->pickedUpFlag();
		}

		if ( pPlayer )
		{
			int iTeam = CTeamFortress2Mod::getTeam(pPlayer);

			if ( CTeamFortress2Mod::isFlagAtDefaultState() )
			{
				CClient *pClient;

				pClient = CClients::get(pPlayer);

				if ( pClient && pClient->autoWaypointOn() )
					pClient->autoEventWaypoint(CWaypointTypes::W_FL_FLAG,200.0f,false);
			}

			CTeamFortress2Mod::flagPickedUp(iTeam,pPlayer);

		}
		

		break;
	case FLAG_CAPTURED: // captured
		{
			IPlayerInfo *p = NULL;
			
			if( pPlayer )
			{
				p = playerinfomanager->GetPlayerInfo(pPlayer);

				if ( p )
				{
					CBroadcastFlagCaptured captured = CBroadcastFlagCaptured(p->GetTeamIndex());
					CBots::botFunction(&captured);
				}
			}

			if ( pBot && pBot->isTF() )
			{
				((CBotTF2*)pBot)->capturedFlag();	
				((CBotTF2*)pBot)->droppedFlag();	
			}
		
			if ( pPlayer )
			{
				int iTeam = CTeamFortress2Mod::getTeam(pPlayer);
				CTeamFortress2Mod::flagDropped(iTeam);

				CClient *pClient;

				pClient = CClients::get(pPlayer);

				if ( pClient && pClient->autoWaypointOn() )
					pClient->autoEventWaypoint(CWaypointTypes::W_FL_CAPPOINT,200.0f,false);
			}

			CTeamFortress2Mod::resetFlagStateToDefault();
			
		}
		break;
	case FLAG_DROPPED: // drop
		{
			IPlayerInfo *p = playerinfomanager->GetPlayerInfo(pPlayer);

			if ( p )
			{
				CBroadcastFlagDropped dropped = CBroadcastFlagDropped(p->GetTeamIndex(),CBotGlobals::entityOrigin(pPlayer));
				CBots::botFunction(&dropped);
			}

			if ( pBot && pBot->isTF() )
				((CBotTF2*)pBot)->droppedFlag();

			
			if ( pPlayer )
				CTeamFortress2Mod::flagDropped(CTeamFortress2Mod::getTeam(pPlayer));
		}
		break;
	case FLAG_RETURN:
		{
			if ( CTeamFortress2Mod::isMapType(TF_MAP_SD) )
			{
				CBroadcastFlagReturned returned = CBroadcastFlagReturned(CTeamFortress2Mod::getFlagCarrierTeam());
				CBots::botFunction(&returned);
			}
			CTeamFortress2Mod::resetFlagStateToDefault();

			CTeamFortress2Mod::flagReturned(0); // for special delivery
			//p->GetTeamIndex(),CBotGlobals::entityOrigin(pPlayer));
		}
		break;
	default:	
		break;
	}

}

void CFlagCaptured :: execute ( IBotEventInterface *pEvent )
{

}
/////////////////////////////////////////////////
void CDODPointCaptured :: execute ( IBotEventInterface *pEvent )
{
	int cp = pEvent->getInt("cp");
	const char *szCappers = pEvent->getString("cappers",NULL);
	edict_t *pPlayer;

	// get a capper
	int userid = szCappers[0];

	int team = 0;

	// find the team - should be a player index
	if ( (userid >= 0) && (userid <= gpGlobals->maxClients) )
	{
		pPlayer = INDEXENT(userid);
		team = CClassInterface::getTeam(pPlayer);

		CClient *pClient = CClients::get(pPlayer);

		if ( pClient && pClient->autoWaypointOn() )
		{
			pClient->autoEventWaypoint(CWaypointTypes::W_FL_CAPPOINT,150.0f,false,0,Vector(0,0,0),true);
		}
	}

	if ( team )
	{
		CBroadcastBombEvent func(DOD_POINT_CAPTURED,cp,team);

		CBots::botFunction(&func);
	}
}

void CDODBombExploded :: execute ( IBotEventInterface *pEvent )
{
	int cp = pEvent->getInt("cp");
	int team = CClassInterface::getTeam(m_pActivator);

	if ( m_pActivator )
	{
		CBroadcastBombEvent func(DOD_BOMB_EXPLODED,cp,team);

		CBots::botFunction(&func);
	}

	CDODMod::m_Flags.setBombPlanted(cp,false);
}

void CDODBombDefused :: execute ( IBotEventInterface *pEvent )
{
	int cp = pEvent->getInt("cp");
	int team = pEvent->getInt("team");

	CDODMod::m_Flags.setBombPlanted(cp,false);

	CBroadcastBombEvent func(DOD_BOMB_DEFUSE,cp,team);

	CBots::botFunction(&func);
}

void CDODBombPlanted :: execute ( IBotEventInterface *pEvent )
{
	int cp = pEvent->getInt("cp");
	int team = pEvent->getInt("team");

	CBroadcastBombEvent func(DOD_BOMB_PLANT,cp,team);

/*	if ( m_pActivator )
	{
		CBot *pBot;

		if ( (pBot = CBots::getBotPointer(m_pActivator)) != NULL )
		{
			// hack
			((CDODBot*)pBot)->removeBomb();
		}
	}*/

	CBots::botFunction(&func);

	CDODMod::m_Flags.setBombPlanted(cp,true);

}

void CDODRoundStart :: execute ( IBotEventInterface *pEvent )
{
	CDODMod::roundStart();
}

void CDODRoundActive :: execute ( IBotEventInterface *pEvent )
{

}

void CDODRoundWin :: execute ( IBotEventInterface *pEvent )
{
	//CDODMod::m_Flags.reset();
}

void CDODRoundOver :: execute ( IBotEventInterface *pEvent )
{
	//CDODMod::m_Flags.reset();
}

void CDODChangeClass :: execute ( IBotEventInterface *pEvent )
{
	if ( m_pActivator )
	{
		CBot *pBot = CBots::getBotPointer(m_pActivator);

		if ( pBot )
		{
			CDODBot *pDODBot = (CDODBot*)pBot;

			pDODBot->selectedClass(pEvent->getInt("class"));
		}
	}
}
///////////////////////////////////////////////////////

void CBotEvent :: setType ( char *szType )
{
	m_szType = CStrings::getString(szType);
}

bool CBotEvent :: forCurrentMod ()
{
	return ((m_iModId == MOD_ANY) || (CBotGlobals::isMod(m_iModId)));
}
// should we execute this ??
inline bool CBotEvent :: isType ( const char *szType )
{
	return forCurrentMod() && FStrEq(m_szType,szType);
}

///////////////////////////////////////////////////////
void CBotEvents :: setupEvents ()
{
	addEvent(new CRoundStartEvent());
	addEvent(new CPlayerHurtEvent());
	addEvent(new CPlayerDeathEvent());
	addEvent(new CBombPickupEvent());
	addEvent(new CPlayerFootstepEvent());
	addEvent(new CBombDroppedEvent());
	addEvent(new CWeaponFireEvent());
	addEvent(new CBulletImpactEvent());
	addEvent(new CFlagEvent());
	addEvent(new CPlayerSpawnEvent());
	////////////// tf2
	addEvent(new CTF2BuiltObjectEvent());
	addEvent(new CTF2ChangeClass());
	addEvent(new CTF2RoundStart());
	addEvent(new CTF2PointCaptured());
	addEvent(new CTF2PointStartCapture());
	addEvent(new CTF2ObjectSapped());
	addEvent(new CTF2ObjectDestroyed());
	addEvent(new CTF2PointStopCapture());
	addEvent(new CTF2PointBlockedCapture());
	addEvent(new CTF2UpgradeObjectEvent());
	addEvent(new CTF2SetupFinished());
	addEvent(new COverTimeBegin());
	addEvent(new CPlayerHealed());
	addEvent(new CPlayerTeleported());
	addEvent(new CDODChangeClass());
	addEvent(new CDODBombPlanted());
	addEvent(new CDODBombExploded());
	addEvent(new CDODBombDefused());
	addEvent(new CDODPointCaptured());
	
/*
pumpkin_lord_summoned 
merasmus_summoned 
eyeball_boss_summoned 

pumpkin_lord_killed 
merasmus_killed 
merasmus_escaped 
eyeball_boss_killed 
eyeball_boss_escaped */

	addEvent(new CBossSummonedEvent("pumpkin_lord_summoned"));
	addEvent(new CBossSummonedEvent("merasmus_summoned"));
	addEvent(new CBossSummonedEvent("eyeball_boss_summoned"));
	addEvent(new CBossKilledEvent("pumpkin_lord_killed"));
	addEvent(new CBossKilledEvent("merasmus_killed"));
	addEvent(new CBossKilledEvent("merasmus_escaped"));
	addEvent(new CBossKilledEvent("eyeball_boss_killed"));
	addEvent(new CBossKilledEvent("eyeball_boss_escaped"));
	addEvent(new CTF2RoundActive());
	addEvent(new CDODRoundStart());
	addEvent(new CDODRoundActive());
	addEvent(new CDODRoundWin());
	addEvent(new CDODRoundOver());
}

void CBotEvents :: addEvent ( CBotEvent *pEvent )
{
	extern IGameEventManager2 *gameeventmanager;
	extern CRCBotPlugin g_RCBOTServerPlugin;

	if ( gameeventmanager )
		gameeventmanager->AddListener( g_RCBOTServerPlugin.getEventListener(), pEvent->getName(), true );

	m_theEvents.push_back(pEvent);
}

void CBotEvents :: freeMemory ()
{
	for ( unsigned int i = 0; i < m_theEvents.size(); i ++ )
	{
		delete m_theEvents[i];
		m_theEvents[i] = NULL;	
	}
	m_theEvents.clear();
}

void CBotEvents :: executeEvent( void *pEvent, eBotEventType iType )
{
	CBotEvent *pFound;
	int iEventId = -1; 
	bool bFound;

	IBotEventInterface *pInterface = NULL;

	if ( iType == TYPE_KEYVALUES )
		pInterface = new CGameEventInterface1((KeyValues*)pEvent);
	else if ( iType == TYPE_IGAMEEVENT )
		pInterface = new CGameEventInterface2((IGameEvent*)pEvent);

	if ( pInterface == NULL )
		return;

	if ( iType != TYPE_IGAMEEVENT )
		iEventId = pInterface->getInt("eventid");

	for ( register unsigned int i = 0; i < m_theEvents.size(); i ++ )
	{
		pFound = m_theEvents[i];

		// if it has an pEvent id stored just check that
		//if ( ( iType != TYPE_IGAMEEVENT ) && pFound->hasEventId() )
		//	bFound = pFound->isEventId(iEventId);
		//else
		bFound = pFound->forCurrentMod() && pFound->isType(pInterface->getName());

		if ( bFound )	
		{
			// set pEvent id for quick checking
			pFound->setEventId(iEventId);

			pFound->setActivator(CBotGlobals::playerByUserId(pInterface->getInt("userid")));

			pFound->execute(pInterface);

			break;
		}
	}

	delete pInterface;
}
