
#include "filesystem.h"
#include "interface.h"
#include "engine/iserverplugin.h"
#include "iplayerinfo.h"
#include "IEngineTrace.h"
#include "tier2/tier2.h"
#ifdef __linux__
#include "shake.h"    //bir3yk
#endif
#include "IEffects.h"
#include "vplane.h"
#include "eiface.h"
#include "igameevents.h"
#include "icvar.h"
//#include "iconvar.h"
#include "convar.h"
#include "Color.h"
#include "ndebugoverlay.h"
#include "server_class.h"
#include "time.h"
#include "bot.h"
#include "bot_getprop.h"
#include "bot_globals.h"

#include "vstdlib/random.h" // for random  seed 

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CClassInterfaceValue CClassInterface :: g_GetProps[GET_PROPDATA_MAX];
bool CClassInterfaceValue :: m_berror = false;

extern IServerGameDLL *servergamedll;

void UTIL_FindServerClassPrint(const char *name_cmd)
{
	bool bInterfaceErr = false;
	char temp[128];
	char name[128];

	strncpy(name,name_cmd,127);
	name[127] = 0;
	strlow(name);

	try
	{
		ServerClass *pClass = servergamedll->GetAllServerClasses();

		while (pClass)
		{
			strncpy(temp,pClass->m_pNetworkName,127);
			temp[127] = 0;

			strlow(temp);

			if (strstr(temp,name) != NULL )
			{
				CBotGlobals::botMessage(NULL,0,"%s",pClass->m_pNetworkName);
				//break;
			}
			pClass = pClass->m_pNext;
		}
	}
	catch (...)
	{
		bInterfaceErr = true;
	}
#ifndef __linux__

	if ( bInterfaceErr )
	{
		// IServerGameDLL_004 == IServerGameDLL except without the replay init function
		ServerClass *pClass = ((IServerGameDLL_004*)servergamedll)->GetAllServerClasses();

		while (pClass)
		{
			strncpy(temp,pClass->m_pNetworkName,127);
			temp[127] = 0;

			strlow(temp);

			if (strstr(temp,name) != NULL )
			{
				CBotGlobals::botMessage(NULL,0,"%s",pClass->m_pNetworkName);
				//break;
			}
			pClass = pClass->m_pNext;
		}
	}
#endif
}
/**
 * Searches for a named Server Class.
 *
 * @param name		Name of the top-level server class.
 * @return 		Server class matching the name, or NULL if none found.
 */
ServerClass *UTIL_FindServerClass(const char *name)
{
	bool bInterfaceErr = false;

	try
	{
		ServerClass *pClass = servergamedll->GetAllServerClasses();

		while (pClass)
		{
			if (strcmpi(pClass->m_pNetworkName, name) == 0)
			{
				return pClass;
			}
			pClass = pClass->m_pNext;
		}
	}
	catch (...)
	{
		bInterfaceErr = true;
	}
#ifndef __linux__
	if ( bInterfaceErr )
	{
		// IServerGameDLL_004 == IServerGameDLL except without the replay init function
		ServerClass *pClass = ((IServerGameDLL_004*)servergamedll)->GetAllServerClasses();

		while (pClass)
		{
			if (strcmp(pClass->m_pNetworkName, name) == 0)
			{
				return pClass;
			}
			pClass = pClass->m_pNext;
		}
	}
#endif
	return NULL;
	
}

/**
 * Recursively looks through a send table for a given named property.
 *
 * @param pTable	Send table to browse.
 * @param name		Property to search for.
 * @return 		SendProp pointer on success, NULL on failure.
 */
bool g_PrintProps = false;

SendProp *UTIL_FindSendProp(SendTable *pTable, const char *name)
{
	int count = pTable->GetNumProps();
	//SendTable *pTable;
	SendProp *pProp;
	for (int i=0; i<count; i++)
	{
		pProp = pTable->GetProp(i);

		if ( g_PrintProps )
			Msg("%s\n",pProp->GetName());

		if (strcmp(pProp->GetName(), name) == 0)
		{
			return pProp;
		}
		if (pProp->GetDataTable())
		{
			if ((pProp=UTIL_FindSendProp(pProp->GetDataTable(), name)) != NULL)
			{
				return pProp;
			}
		}
	}
 
	return NULL;
}
/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

struct sm_sendprop_info_t
{
	SendProp *prop;					/**< Property instance. */
	unsigned int actual_offset;		/**< Actual computed offset. */
};

bool UTIL_FindInSendTable(SendTable *pTable, 
						  const char *name,
						  sm_sendprop_info_t *info,
						  unsigned int offset)
{
	const char *pname;
	int props = pTable->GetNumProps();
	SendProp *prop;

	for (int i=0; i<props; i++)
	{
		prop = pTable->GetProp(i);
		pname = prop->GetName();

		if ( g_PrintProps )
			Msg("%d : %s\n",offset + prop->GetOffset(),pname);

		if (pname && strcmp(name, pname) == 0)
		{
			info->prop = prop;
			// for some reason offset is sometimes negative when it shouldn't be
			// so take the absolute value
			info->actual_offset = offset + abs(info->prop->GetOffset());
			return true;
		}
		if (prop->GetDataTable())
		{
			if (UTIL_FindInSendTable(prop->GetDataTable(), 
				name,
				info,
				offset + prop->GetOffset())
				)
			{
				return true;
			}
		}
	}

	return false;
}

bool UTIL_FindSendPropInfo(ServerClass *pInfo, const char *szType, unsigned int *offset)
{
	if ( !pInfo )
	{
		return false;
	}

	sm_sendprop_info_t temp_info;

	if (!UTIL_FindInSendTable(pInfo->m_pTable, szType, &temp_info, 0))
	{
		return false;
	}

	*offset = temp_info.actual_offset;

	return true;
}

CBaseHandle *CClassInterfaceValue :: getEntityHandle ( edict_t *edict ) 
{ 
	getData(edict); 

	return (CBaseHandle *)m_data;
}

edict_t *CClassInterfaceValue :: getEntity ( edict_t *edict ) 
{ 
	static CBaseHandle *hndl;
	getData(edict); 

	hndl = (CBaseHandle *)m_data; 

	if ( hndl )
		return INDEXENT(hndl->GetEntryIndex());

	return NULL;
}

void CClassInterfaceValue :: init ( char *key, char *value, unsigned int preoffset )
{
	m_class = CStrings::getString(key);
	m_value = CStrings::getString(value);
	m_data = NULL;
	m_preoffset = preoffset;
	m_offset = 0;
}

void CClassInterfaceValue :: findOffset ( )
{
	//if (!m_offset)
	//{
	ServerClass *sc = UTIL_FindServerClass(m_class);

	if ( sc )
		UTIL_FindSendPropInfo(sc,m_value,&m_offset);
	//}

	if ( m_offset > 0 )
		m_offset += m_preoffset;
}

void CClassInterface:: init ()
{
		DEFINE_GETPROP(GETPROP_TF2MINIBUILDING,"CObjectSentryGun","m_bMiniBuilding",0);
		DEFINE_GETPROP(GETPROP_TF2SCORE,"CTFPlayerResource","m_iTotalScore",0);
		DEFINE_GETPROP(GETPROP_ENTITY_FLAGS,"CBaseEntity","m_iEffectFlags",0);
		DEFINE_GETPROP(GETPROP_TEAM,"CBaseEntity","m_iTeamNum",0);
		DEFINE_GETPROP(GETPROP_PLAYERHEALTH,"CBasePlayer","m_iHealth",0);
		DEFINE_GETPROP(GETPROP_EFFECTS,"CBaseEntity","m_fEffects",0);
		DEFINE_GETPROP(GETPROP_AMMO,"CBasePlayer","m_iAmmo",0);
		DEFINE_GETPROP(GETPROP_TF2_NUMHEALERS,"CTFPlayer","m_nNumHealers",4);
		DEFINE_GETPROP(GETPROP_TF2_CONDITIONS,"CTFPlayer","m_nPlayerCond",0);
		DEFINE_GETPROP(GETPROP_VELOCITY,"CBasePlayer","m_vecVelocity[0]",0);
		DEFINE_GETPROP(GETPROP_TF2CLASS,"CTFPlayer","m_PlayerClass",4);
		DEFINE_GETPROP(GETPROP_TF2SPYMETER,"CTFPlayer","m_flCloakMeter",0);
		DEFINE_GETPROP(GETPROP_TF2SPYDISGUISED_TEAM,"CTFPlayer","m_nDisguiseTeam",0);
		DEFINE_GETPROP(GETPROP_TF2SPYDISGUISED_CLASS,"CTFPlayer","m_nDisguiseClass",0);
		DEFINE_GETPROP(GETPROP_TF2SPYDISGUISED_TARGET_INDEX,"CTFPlayer","m_iDisguiseTargetIndex",0);
		DEFINE_GETPROP(GETPROP_TF2SPYDISGUISED_DIS_HEALTH,"CTFPlayer","m_iDisguiseHealth",0);
		DEFINE_GETPROP(GETPROP_TF2MEDIGUN_HEALING,"CWeaponMedigun","m_bHealing",0);
		DEFINE_GETPROP(GETPROP_TF2MEDIGUN_TARGETTING,"CWeaponMedigun","m_hHealingTarget",0);
		DEFINE_GETPROP(GETPROP_TF2TELEPORTERMODE,"CObjectTeleporter","m_iObjectMode",0);
		DEFINE_GETPROP(GETPROP_CURRENTWEAPON,"CBaseCombatCharacter","m_hActiveWeapon",0);
		DEFINE_GETPROP(GETPROP_TF2UBERCHARGE_LEVEL,"CWeaponMedigun","m_flChargeLevel",0);
		DEFINE_GETPROP(GETPROP_TF2SENTRYHEALTH,"CObjectSentrygun","m_iHealth",0);
		DEFINE_GETPROP(GETPROP_TF2DISPENSERHEALTH,"CObjectDispenser","m_iHealth",0);
		DEFINE_GETPROP(GETPROP_TF2TELEPORTERHEALTH,"CObjectTeleporter","m_iHealth",0);
		DEFINE_GETPROP(GETPROP_TF2OBJECTCARRIED,"CObjectSentrygun","m_bCarried",0);
		DEFINE_GETPROP(GETPROP_TF2OBJECTUPGRADELEVEL,"CObjectSentrygun","m_iUpgradeLevel",0);
		DEFINE_GETPROP(GETPROP_TF2OBJECTMAXHEALTH,"CObjectSentrygun","m_iMaxHealth",0);
		DEFINE_GETPROP(GETPROP_TF2DISPMETAL,"CObjectDispenser","m_iAmmoMetal",0);
		DEFINE_GETPROP(GETPROP_MAXSPEED,"CBasePlayer","m_flMaxspeed",0);
		DEFINE_GETPROP(GETPROP_CONSTRAINT_SPEED,"CTFPlayer","m_flConstraintSpeedFactor",0);
		DEFINE_GETPROP(GETPROP_TF2OBJECTBUILDING,"CObjectDispenser","m_bBuilding",0);
		DEFINE_GETPROP(GETPROP_ENTITYFLAGS,"CBasePlayer","m_fFlags",0);

		// hl2dm
		DEFINE_GETPROP(GETPROP_HL2DM_PHYSCANNON_ATTACHED,"CWeaponPhysCannon","m_hAttachedObject",0);
		DEFINE_GETPROP(GETPROP_HL2DM_PHYSCANNON_OPEN,"CWeaponPhysCannon","m_bOpen",0);
		DEFINE_GETPROP(GETPROP_HL2DM_PLAYER_AUXPOWER,"CHL2MP_Player","m_flSuitPower",0);
		DEFINE_GETPROP(GETPROP_HL2DM_LADDER_ENT,"CHL2MP_Player","m_hLadder",0);
		
		DEFINE_GETPROP(GETPROP_WEAPONLIST,"CBaseCombatCharacter","m_hMyWeapons",0);
		DEFINE_GETPROP(GETPROP_WEAPONSTATE,"CBaseCombatWeapon","m_iState",0);

		DEFINE_GETPROP(GETPROP_WEAPONCLIP1,"CBaseCombatWeapon","m_iClip1",0);
		DEFINE_GETPROP(GETPROP_WEAPONCLIP2,"CBaseCombatWeapon","m_iClip2",0);

		DEFINE_GETPROP(GETPROP_WEAPON_AMMOTYPE1,"CBaseCombatWeapon","m_iPrimaryAmmoType",0);
		DEFINE_GETPROP(GETPROP_WEAPON_AMMOTYPE2,"CBaseCombatWeapon","m_iSecondaryAmmoType",0);

		DEFINE_GETPROP(GETPROP_DOD_PLAYERCLASS,"CDODPlayer","m_iPlayerClass",0);
		DEFINE_GETPROP(GETPROP_DOD_DES_PLAYERCLASS,"CDODPlayer","m_iDesiredPlayerClass",0);

		DEFINE_GETPROP(GETPROP_DOD_STAMINA,"CDODPlayer","m_flStamina",0);
		DEFINE_GETPROP(GETPROP_DOD_PRONE,"CDODPlayer","m_bProne",0);
		DEFINE_GETPROP(GETPROP_SEQUENCE,"CBaseAnimating","m_nSequence",0);
		DEFINE_GETPROP(GETPROP_CYCLE,"CBaseAnimating","m_flCycle",0);

		DEFINE_GETPROP(GETPROP_DOD_CP_NUMCAPS,"CDODObjectiveResource","m_iNumControlPoints",0);
		DEFINE_GETPROP(GETPROP_DOD_CP_POSITIONS,"CDODObjectiveResource","m_vCPPositions",0);
		DEFINE_GETPROP(GETPROP_DOD_CP_ALLIES_REQ_CAP,"CDODObjectiveResource","m_iAlliesReqCappers",0);
		DEFINE_GETPROP(GETPROP_DOD_CP_AXIS_REQ_CAP,"CDODObjectiveResource","m_iAxisReqCappers",0);
		DEFINE_GETPROP(GETPROP_DOD_CP_NUM_AXIS,"CDODObjectiveResource","m_iNumAxis",0);
		DEFINE_GETPROP(GETPROP_DOD_CP_NUM_ALLIES,"CDODObjectiveResource","m_iNumAllies",0);
		DEFINE_GETPROP(GETPROP_DOD_CP_OWNER,"CDODObjectiveResource","m_iOwner",0);
		DEFINE_GETPROP(GETPROP_DOD_SNIPER_ZOOMED,"CDODSniperWeapon","m_bZoomed",0);
		DEFINE_GETPROP(GETPROP_DOD_MACHINEGUN_DEPLOYED,"CDODBipodWeapon","m_bDeployed",0);
		DEFINE_GETPROP(GETPROP_DOD_ROCKET_DEPLOYED,"CDODBaseRocketWeapon","m_bDeployed",0);
		DEFINE_GETPROP(GETPROP_DOD_SEMI_AUTO,"CDODFireSelectWeapon","m_bSemiAuto",0);
		DEFINE_GETPROP(GETPROP_MOVETYPE,"CBaseEntity","movetype",0);
		DEFINE_GETPROP(GETPROP_DOD_GREN_THROWER,"CDODBaseGrenade","m_hThrower",0);
		DEFINE_GETPROP(GETPROP_DOD_SCORE,"CDODPlayerResource","m_iScore",0);
		DEFINE_GETPROP(GETPROP_DOD_OBJSCORE,"CDODPlayerResource","m_iObjScore",0);
		DEFINE_GETPROP(GETPROP_DOD_DEATHS,"CDODPlayerResource","m_iDeaths",0);
		DEFINE_GETPROP(GETPROP_DOD_SMOKESPAWN_TIME,"CDODSmokeGrenade","m_flSmokeSpawnTime",0);
		DEFINE_GETPROP(GETPROP_DOD_ROUNDTIME,"CDODGameRulesProxy","m_flRestartRoundTime",0);
		DEFINE_GETPROP(GETPROP_DOD_K98ZOOM,"CWeaponK98","m_bZoomed",0);
		DEFINE_GETPROP(GETPROP_DOD_GARANDZOOM,"CWeaponGarand","m_bZoomed",0);
		DEFINE_GETPROP(GETPROP_DOD_ALLIESBOMBING,"CDODGameRulesProxy","m_bAlliesAreBombing",0);
		DEFINE_GETPROP(GETPROP_DOD_AXISBOMBING,"CDODGameRulesProxy","m_bAxisAreBombing",0);
		DEFINE_GETPROP(GETPROP_DOD_BOMBSPLANTED,"CDODObjectiveResource","m_bBombPlanted",0);
		DEFINE_GETPROP(GETPROP_DOD_BOMBSREQ,"CDODObjectiveResource","m_iBombsRequired",0);
		DEFINE_GETPROP(GETPROP_DOD_BOMBSDEFUSED,"CDODObjectiveResource","m_bBombBeingDefused",0);
		DEFINE_GETPROP(GETPROP_DOD_BOMBSREMAINING,"CDODObjectiveResource","m_iBombsRemaining",0);
		DEFINE_GETPROP(GETPROP_DOD_PLANTINGBOMB,"CDODPlayer","m_bPlanting",0);
		DEFINE_GETPROP(GETPROP_DOD_DEFUSINGBOMB,"CDODPlayer","m_bDefusing",0);
		DEFINE_GETPROP(GETPROP_DOD_BOMB_STATE,"CDODBombTarget","m_iState",0);
		DEFINE_GETPROP(GETPROP_DOD_BOMB_TEAM,"CDODBombTarget","m_iBombingTeam",0);
		DEFINE_GETPROP(GETPROP_DOD_CP_VISIBLE,"CDODObjectiveResource","m_bCPIsVisible",0);

		DEFINE_GETPROP(GETPROP_ALL_ENTOWNER,"CBaseEntity","m_hOwnerEntity",0);
		DEFINE_GETPROP(GETPROP_GROUND_ENTITY,"CBasePlayer","m_hGroundEntity",0);

		for ( unsigned int i = 0; i < GET_PROPDATA_MAX; i ++ )
		{
			//if ( g_GetProps[i]
			g_GetProps[i].findOffset();
		}
}

void CClassInterfaceValue :: getData ( edict_t *edict )
{
	static IServerUnknown *pUnknown;
	static CBaseEntity *pEntity;

	if (!m_offset)
	{
		m_data = NULL;
		m_berror = true;
		return;
	}

	pUnknown = (IServerUnknown *)edict->GetUnknown();

	if (!pUnknown)
	{
		m_data = NULL;
		m_berror = true;
		return;
	}
 
	pEntity = pUnknown->GetBaseEntity();

	m_data = (void *)((char *)pEntity + m_offset);
}

edict_t *CClassInterface::FindEntityByClassnameNearest(Vector vstart, const char *classname, float fMindist, edict_t *pOwner)
{
	edict_t *current;
	edict_t *pfound = NULL;
	float fDist;
	// speed up loop by by using smaller ints in register
	register short int max = (short int)gpGlobals->maxEntities;

	for (register short int i = 0; i < max; i++)
	{
		current = engine->PEntityOfEntIndex(i);

		if (current == NULL)
			continue;

		if ( current->IsFree() )
			continue;

		if ( pOwner != NULL )
		{
			if ( getOwner(current) != pOwner )
				continue;
		}

		if (strcmp(classname, current->GetClassName() ) == 0)
		{
			fDist = (vstart - CBotGlobals::entityOrigin(current)).Length();

			if ( !pfound  || (fDist < fMindist))
			{
				fMindist = fDist;
				pfound = current;
			}
		}
	}

	return pfound;
}

edict_t *CClassInterface::FindEntityByNetClassNearest(Vector vstart, const char *classname)
{
	edict_t *current;
	edict_t *pfound = NULL;
	float fMindist = 8192.0f;
	float fDist;

	for (short int i = 0; i < gpGlobals->maxEntities; i++)
	{
		current = engine->PEntityOfEntIndex(i);
		if (current == NULL)
		{
			continue;
		}

		IServerNetworkable *network = current->GetNetworkable();

		if (network == NULL)
		{
			continue;
		}

		ServerClass *sClass = network->GetServerClass();
		const char *name = sClass->GetName();
		
		if (strcmp(name, classname) == 0)
		{
			fDist = (vstart - CBotGlobals::entityOrigin(current)).Length();

			if ( !pfound  || (fDist < fMindist))
			{
				fMindist = fDist;
				pfound = current;
			}
		}
	}

	return pfound;
}
// http://svn.alliedmods.net/viewvc.cgi/trunk/extensions/tf2/extension.cpp?revision=2183&root=sourcemod&pathrev=2183
edict_t *CClassInterface::FindEntityByNetClass(int start, const char *classname)
{
	edict_t *current;

	for (int i = ((start != -1) ? start : 0); i < gpGlobals->maxEntities; i++)
	{
		current = engine->PEntityOfEntIndex(i);
		if (current == NULL)
		{
			continue;
		}

		IServerNetworkable *network = current->GetNetworkable();

		if (network == NULL)
		{
			continue;
		}

		ServerClass *sClass = network->GetServerClass();
		const char *name = sClass->GetName();
		

		if (strcmp(name, classname) == 0)
		{
			return current;
		}
	}

	return NULL;
}


 int CClassInterface::getTF2Score ( edict_t *edict ) 
	{ 
		edict_t *res = CTeamFortress2Mod::findResourceEntity();
		int *score_array = NULL;

		if ( res )
		{
			score_array = g_GetProps[GETPROP_TF2SCORE].getIntPointer(res);

			if ( score_array )
				return score_array[ENTINDEX(edict)-1];
		}

		return 0;
	}
