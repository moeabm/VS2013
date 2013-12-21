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
#include "server_class.h"

#include "bot.h"

#include "in_buttons.h"

#include "bot_mods.h"
#include "bot_globals.h"
#include "bot_fortress.h"
#include "bot_weapons.h"
#include "bot_script.h"
#include "bot_configfile.h"
#include "bot_getprop.h"
#include "bot_dod_bot.h"
#include "bot_navigator.h"
#include "bot_waypoint.h"
#include "bot_waypoint_locations.h"
#include "bot_perceptron.h"

eTFMapType CTeamFortress2Mod :: m_MapType = TF_MAP_CTF;
tf_tele_t CTeamFortress2Mod :: m_Teleporters[MAX_PLAYERS];
int CTeamFortress2Mod :: m_iArea = 0;
float CTeamFortress2Mod::m_fSetupTime = 0.0f;
float CTeamFortress2Mod::m_fRoundTime = 0.0f;
MyEHandle CTeamFortress2Mod::m_pFlagCarrierRed = MyEHandle(NULL);
MyEHandle CTeamFortress2Mod::m_pFlagCarrierBlue = MyEHandle(NULL);
float CTeamFortress2Mod::m_fArenaPointOpenTime = 0.0f;
float CTeamFortress2Mod::m_fPointTime = 0.0f;
tf_sentry_t CTeamFortress2Mod::m_SentryGuns[MAX_PLAYERS];	// used to let bots know if sentries have been sapped or not
tf_disp_t  CTeamFortress2Mod::m_Dispensers[MAX_PLAYERS];	// used to let bots know where friendly/enemy dispensers are
MyEHandle CTeamFortress2Mod::m_pResourceEntity = MyEHandle(NULL);
bool CTeamFortress2Mod::m_bAttackDefendMap = false;
//float g_fBotUtilityPerturb [TF_CLASS_MAX][BOT_UTIL_MAX];
int CTeamFortress2Mod::m_Cappers[MAX_CAP_POINTS];
bool CTeamFortress2Mod::m_bHasRoundStarted = true;
int CTeamFortress2Mod::m_iFlagCarrierTeam = 0;
MyEHandle CTeamFortress2Mod::m_pBoss = MyEHandle(NULL);
bool CTeamFortress2Mod::m_bBossSummoned = false;
MyEHandle CTeamFortress2Mod::pMediGuns[MAX_PLAYERS];
edict_t *CDODMod::m_pResourceEntity = NULL;
CDODFlags CDODMod::m_Flags;
edict_t * CDODMod::m_pPlayerResourceEntity = NULL;
float CDODMod::m_fMapStartTime = 0.0f;
edict_t * CDODMod::m_pGameRules = NULL;
int CDODMod::m_iMapType = 0;
bool CDODMod::m_bCommunalBombPoint = false;
int CDODMod::m_iBombAreaAllies = 0;
int CDODMod::m_iBombAreaAxis = 0;
bool CTeamFortress2Mod::bFlagStateDefault = true;
//CPerceptron *CDODMod::gNetAttackOrDefend = NULL;
float CDODMod::fAttackProbLookUp[MAX_DOD_FLAGS+1][MAX_DOD_FLAGS+1];
vector<edict_wpt_pair_t> CDODMod::m_BombWaypoints;
vector<edict_wpt_pair_t> CDODMod::m_BreakableWaypoints;
vector<edict_wpt_pair_t> CHalfLifeDeathmatchMod::m_LiftWaypoints;

extern ConVar bot_use_disp_dist;

eDODVoiceCommand_t g_DODVoiceCommands[DOD_VC_INVALID] = 
{
	{DOD_VC_GOGOGO,"attack"},
	{DOD_VC_YES,"yessir"},
	{DOD_VC_DROPWEAP,"dropweapons"},
	{DOD_VC_HOLD,"hold"},
	{DOD_VC_NO,"negative"},
	{DOD_VC_DISPLACE,"displace"},
	{DOD_VC_GO_LEFT,"flankleft"},
	{DOD_VC_NEED_BACKUP,"backup"},
	{DOD_VC_MGAHEAD,"mgahead"},
	{DOD_VC_GO_RIGHT,"flankright"},
	{DOD_VC_FIRE_IN_THE_HOLE,"fireinhole"},
	{DOD_VC_ENEMY_BEHIND,"enemybehind"},
	{DOD_VC_STICK_TOGETHER,"sticktogether"},
	{DOD_VC_USE_GRENADE,"usegrens"},
	{DOD_VC_ENEMY_DOWN,"wegothim"},
	{DOD_VC_COVERING_FIRE,"cover"},
	{DOD_VC_SNIPER,"sniper"},
	{DOD_VC_NEED_MG,"moveupmg"},
	{DOD_VC_SMOKE,"usesmoke"},
	{DOD_VC_NICE_SHOT,"niceshot"},
	{DOD_VC_NEED_AMMO,"needammo"},
	{DOD_VC_GRENADE2,"grenade"},
	{DOD_VC_THANKS,"thanks"},
	{DOD_VC_USE_BAZOOKA,"usebazooka"},
	{DOD_VC_CEASEFIRE,"ceasefire"},
	{DOD_VC_AREA_CLEAR,"areaclear"},
	{DOD_VC_BAZOOKA,"bazookaspotted"}
};


edict_t *CTeamFortress2Mod:: getMediGun ( edict_t *pPlayer )
{
	if ( CClassInterface::getTF2Class(pPlayer) == TF_CLASS_MEDIC )
		return pMediGuns[ENTINDEX(pPlayer)-1];
	return NULL;
}

void CTeamFortress2Mod :: findMediGun ( edict_t *pPlayer )
{
	static int i;
	static edict_t *pEnt;
	static Vector vOrigin;

	vOrigin = CBotGlobals::entityOrigin(pPlayer);

	for ( i = (gpGlobals->maxClients+1); i <= gpGlobals->maxEntities; i ++ )
	{
		pEnt = INDEXENT(i);

		if ( pEnt && CBotGlobals::entityIsValid(pEnt) )
		{
			if ( strcmp(pEnt->GetClassName(),"tf_weapon_medigun") == 0 )
			{
				if ( CBotGlobals::entityOrigin(pEnt) == vOrigin )
				{
					pMediGuns[ENTINDEX(pPlayer)-1] = pEnt;
					break;
				}
			}
		}
	}
}

// get the teleporter exit of an entrance
edict_t *CTeamFortress2Mod :: getTeleporterExit ( edict_t *pTele )
{
	int i;
	edict_t *pExit;

	for ( i = 0; i < MAX_PLAYERS; i ++ )
	{
		if ( m_Teleporters[i].entrance.get() == pTele )
		{
			if ( (pExit = m_Teleporters[i].exit.get()) != NULL )
			{
				return pExit;
			}

			return false;
		}
	}

	return false;
}

// check if the entity is a health kit
bool CTeamFortress2Mod :: isHealthKit ( edict_t *pEntity )
{
	return strncmp(pEntity->GetClassName(),"item_healthkit",14)==0;
}

// cehc kif the team can pick up a flag in SD mode (special delivery)
bool CTeamFortress2Mod::canTeamPickupFlag_SD(int iTeam,bool bGetUnknown)
{
	if ( isArenaPointOpen() )
	{
		if ( bGetUnknown )
			return (m_iFlagCarrierTeam==iTeam);
		else
			return (m_iFlagCarrierTeam==0);
	}

	return false;
}
// for special delivery mod
void CTeamFortress2Mod::flagReturned(int iTeam)
{
	m_iFlagCarrierTeam = 0;
	bFlagStateDefault = true;
}

void CTeamFortress2Mod:: flagPickedUp (int iTeam, edict_t *pPlayer)
{
	bFlagStateDefault = false;
	if ( iTeam == TF2_TEAM_BLUE )
		m_pFlagCarrierBlue = pPlayer;
	else if ( iTeam == TF2_TEAM_RED )
		m_pFlagCarrierRed = pPlayer;

	m_iFlagCarrierTeam = iTeam;

	CBotTF2FunctionEnemyAtIntel *function = new CBotTF2FunctionEnemyAtIntel(iTeam,CBotGlobals::entityOrigin(pPlayer),EVENT_FLAG_PICKUP);

	CBots::botFunction(function);

	delete function;
}

bool CTeamFortress2Mod :: isArenaPointOpen ()
{
	return m_fArenaPointOpenTime < engine->Time();
}

void CTeamFortress2Mod :: resetSetupTime ()
{
	m_fRoundTime = engine->Time() + m_fSetupTime;
	m_fArenaPointOpenTime = engine->Time() + m_fPointTime;
}

bool CTeamFortress2Mod::hasRoundStarted ()
{
	return m_bHasRoundStarted || (engine->Time() > m_fRoundTime);

	//return (engine->Time() > m_fRoundTime);
}

void CTeamFortress2Mod :: setPointOpenTime ( int time )
{
	m_fArenaPointOpenTime = 0.0f;
	m_fPointTime = (float)time;
}

void CTeamFortress2Mod :: setSetupTime ( int time )
{
  m_fRoundTime = 0.0f;
  m_fSetupTime = (float)time;
}

bool CTeamFortress2Mod :: isAmmo ( edict_t *pEntity )
{
	static const char *szClassname;

	szClassname = pEntity->GetClassName();

	return (strcmp(szClassname,"tf_ammo_pack")==0) || (strncmp(szClassname,"item_ammopack",13)==0);
}

bool CTeamFortress2Mod :: isPayloadBomb ( edict_t *pEntity, int iTeam )
{
	return ((strncmp(pEntity->GetClassName(),"mapobj_cart_dispenser",21)==0) && (CClassInterface::getTeam(pEntity)==iTeam));
}


// check voice commands
void CTeamFortress2Mod:: clientCommand ( edict_t *pEntity, int argc, const char *pcmd, const char *arg1, const char *arg2 )
{
	if ( argc > 2 )
	{
		if ( strcmp(pcmd,"voicemenu") == 0 )
		{
			// somebody said a voice command
			u_VOICECMD vcmd;

			vcmd.voicecmd = 0;
			vcmd.b1.v1 = atoi(arg1);
			vcmd.b1.v2 = atoi(arg2);

			CBroadcastVoiceCommand voicecmd = CBroadcastVoiceCommand(pEntity,vcmd.voicecmd); 

			CBots::botFunction(&voicecmd);
		}
	}
}

// to fixed
void CTeamFortress2Mod :: teleporterBuilt ( edict_t *pOwner, eEngiBuild type, edict_t *pBuilding )
{
	int team;

	if ( (type != ENGI_TELE ) ) //(type != ENGI_ENTRANCE) && (type != ENGI_EXIT) )
		return;

	short int iIndex = ENTINDEX(pOwner)-1;

	if ( (iIndex < 0) || (iIndex > gpGlobals->maxClients) )
		return;

	team = getTeam(pOwner);

	if ( CTeamFortress2Mod::isTeleporterEntrance(pBuilding,team) )
		m_Teleporters[iIndex].entrance = MyEHandle(pBuilding);
	else if ( CTeamFortress2Mod::isTeleporterExit(pBuilding,team) )
		m_Teleporters[iIndex].exit = MyEHandle(pBuilding);

	m_Teleporters[iIndex].sapper = MyEHandle();
}
// used for changing class if I'm doing badly in my team
int CTeamFortress2Mod ::getHighestScore ()
{
	short int highest = 0;
	short int score;
	short int i = 0;
	edict_t *edict;

	for ( i = 1; i <= gpGlobals->maxClients; i ++ )
	{
		edict = INDEXENT(i);

		if ( edict && CBotGlobals::entityIsValid(edict) )
		{
			score = (short int)CClassInterface::getTF2Score(edict);
		
			if ( score > highest )
			{
				highest = score;
			}
		}
	}

	return highest;
}

// check if there is another building near where I want to build
// check quickly by using the storage of sentryguns etc in the mod class
bool CTeamFortress2Mod::buildingNearby ( int iTeam, Vector vOrigin )
{
	edict_t *pPlayer;
	short int i;

		for ( i = 1; i <= gpGlobals->maxClients; i ++ )
		{
			pPlayer = INDEXENT(i);

			// crash bug fix 
			if ( !pPlayer || pPlayer->IsFree() )
				continue;

			if ( CClassInterface::getTF2Class(pPlayer) != TF_CLASS_ENGINEER )
				continue;

			if ( CClassInterface::getTeam(pPlayer) != iTeam )
				continue;

			if ( m_SentryGuns[i].sentry.get() )
			{
				if ( (vOrigin-CBotGlobals::entityOrigin(m_SentryGuns[i].sentry.get())).Length() < 100 )
					return true;
			}

			if ( m_Dispensers[i].disp.get() )
			{
				if ( (vOrigin-CBotGlobals::entityOrigin(m_Dispensers[i].disp.get())).Length() < 100 )
					return true;
			}

			if ( m_Teleporters[i].entrance.get() )
			{
				if ( (vOrigin-CBotGlobals::entityOrigin(m_Teleporters[i].entrance.get())).Length() < 100 )
					return true;
			}

			if ( m_Teleporters[i].exit.get() )
			{
				if ( (vOrigin-CBotGlobals::entityOrigin(m_Teleporters[i].exit.get())).Length() < 100 )
					return true;
			}

		}

		return false;
}

// get the owner of 
edict_t *CTeamFortress2Mod ::getBuildingOwner (eEngiBuild object, short index)
{
	static short int i;
	static tf_tele_t *tele;

	switch ( object )
	{
	case ENGI_DISP:
		for ( i = 0; i < MAX_PLAYERS; i ++ )
		{
			if ( m_Dispensers[i].disp.get() && (ENTINDEX(m_Dispensers[i].disp.get())==index) )
				return INDEXENT(i+1);
		}
		//m_SentryGuns[i].
		break;
	case ENGI_SENTRY:
		for ( i = 0; i < MAX_PLAYERS; i ++ )
		{
			if ( m_SentryGuns[i].sentry.get() && (ENTINDEX(m_SentryGuns[i].sentry.get())==index) )
				return INDEXENT(i+1);
		}
		break;
	case ENGI_TELE:
		tele = m_Teleporters;

		for ( i = 0; i < MAX_PLAYERS; i ++ )
		{
			if ( tele->entrance.get() && (ENTINDEX(tele->entrance.get())==index) )
				return INDEXENT(i+1);
			if ( tele->exit.get() && (ENTINDEX(tele->exit.get())==index) )
				return INDEXENT(i+1);

			tele++;
		}
		break;
	}

	return NULL;
}

edict_t *CTeamFortress2Mod :: nearestDispenser ( Vector vOrigin, int team )
{
	edict_t *pNearest = NULL;
	edict_t *pDisp;
	float fDist;
	float fNearest = bot_use_disp_dist.GetFloat();

	for ( unsigned int i = 0; i < MAX_PLAYERS; i ++ )
	{
		//m_Dispensers[i]
		pDisp = m_Dispensers[i].disp.get();

		if ( pDisp )
		{
			if ( CTeamFortress2Mod::getTeam(pDisp) == team )
			{
				fDist = (CBotGlobals::entityOrigin(pDisp) - vOrigin).Length();

				if ( fDist < fNearest )
				{
					pNearest = pDisp;
					fNearest = fDist;
				}
			}
		}
	}

	return pNearest;
}

void CTeamFortress2Mod::sapperPlaced(edict_t *pOwner,eEngiBuild type,edict_t *pSapper)
{
	static short int index;
	
	index = ENTINDEX(pOwner)-1;

	if ( (index>=0) && (index<MAX_PLAYERS) )
	{
		if ( type == ENGI_TELE )
			m_Teleporters[index].sapper = MyEHandle(pSapper);
		else if ( type == ENGI_DISP )
			m_Dispensers[index].sapper = MyEHandle(pSapper);
		else if ( type == ENGI_SENTRY )
			m_SentryGuns[index].sapper = MyEHandle(pSapper);
	}
}

void CTeamFortress2Mod:: addWaypointFlags (edict_t *pPlayer, edict_t *pEdict, int *iFlags, int *iArea, float *fMaxDistance )
{
	string_t model = pEdict->GetIServerEntity()->GetModelName();

	if ( strncmp(pEdict->GetClassName(),"item_health",11) == 0 )
		*iFlags |= CWaypointTypes::W_FL_HEALTH;
	else if ( strncmp(pEdict->GetClassName(),"item_ammo",9) == 0 )
		*iFlags |= CWaypointTypes::W_FL_AMMO;
	else if ( strcmp(pEdict->GetClassName(),"prop_dynamic") == 0 )
	{
		if ( strcmp(model.ToCStr(),"models/props_gameplay/resupply_locker.mdl") == 0 )
		{
			*iFlags |= CWaypointTypes::W_FL_RESUPPLY;

			if ( CTeamFortress2Mod::getTeam(pPlayer) == TF2_TEAM_BLUE )
				*iFlags |= CWaypointTypes::W_FL_NORED;
			else if ( CTeamFortress2Mod::getTeam(pPlayer) == TF2_TEAM_RED )
				*iFlags |= CWaypointTypes::W_FL_NOBLU;
		}
	}
	// do this in the event code
	/*else if ( strcmp(pEdict->GetClassName(),"item_teamflag") == 0 )
	{
		if ( !CTeamFortress2Mod::isFlagCarrier(pPlayer) )
			*iFlags |= CWaypointTypes::W_FL_FLAG;
	}*/
	else if ( strcmp(pEdict->GetClassName(),"team_control_point") == 0 )
	{
		*iFlags |= CWaypointTypes::W_FL_CAPPOINT;	
		*fMaxDistance = 100;
	}
}

void CTeamFortress2Mod::sapperDestroyed(edict_t *pOwner,eEngiBuild type, edict_t *pSapper)
{
	static short int index; 

	for ( index = 0; index < MAX_PLAYERS; index ++ )
	{
		if ( type == ENGI_TELE )
		{
			if ( m_Teleporters[index].sapper.get_old() == pSapper )
				m_Teleporters[index].sapper = MyEHandle();
			
		}
		else if ( type == ENGI_DISP )
		{
			if ( m_Dispensers[index].sapper.get_old() == pSapper )
				m_Dispensers[index].sapper = MyEHandle();
		}
		else if ( type == ENGI_SENTRY )
		{
			if ( m_SentryGuns[index].sapper.get_old() == pSapper )
				m_SentryGuns[index].sapper = MyEHandle();
		}
	}
}

void CTeamFortress2Mod::sentryBuilt(edict_t *pOwner, eEngiBuild type, edict_t *pBuilding )
{
	static short int index;
	static tf_sentry_t *temp;
	
	index = ENTINDEX(pOwner)-1;

	if ( (index>=0) && (index<MAX_PLAYERS) )
	{
		if ( type == ENGI_SENTRY )
		{
			temp = &(m_SentryGuns[index]);
			temp->sentry = MyEHandle(pBuilding);
			temp->sapper = MyEHandle();
			//m_SentryGuns[index].builder
		}
	}
}

bool CTeamFortress2Mod::isSentryGun (edict_t *pEdict )
{
	static short int i;
	static tf_sentry_t *temp;

	temp = m_SentryGuns;

	for ( i = 0; i < MAX_PLAYERS; i ++ )
	{
		if ( temp->sentry == pEdict )
			return true;

		temp++;
	}

	return false;
}

void CTeamFortress2Mod::dispenserBuilt(edict_t *pOwner, eEngiBuild type, edict_t *pBuilding )
{
	static short int index;
	static tf_disp_t *temp;
	
	index = ENTINDEX(pOwner)-1;

	if ( (index>=0) && (index<MAX_PLAYERS) )
	{
		if ( type == ENGI_DISP )
		{
			temp = &(m_Dispensers[index]);
			temp->disp = MyEHandle(pBuilding);
			temp->sapper = MyEHandle();
			//m_Dispensers[index].builder = userid;
		}
	}
}
//
//
//
void CBotMods :: parseFile ()
{
	char buffer[1024];
	unsigned int len;
	char key[64];
	unsigned int i,j;
	char val[256];

	eModId modtype;
	eBotType bottype;
	char steamfolder[256];
	char gamefolder[256];

	CBotGlobals::buildFileName(buffer,BOT_MOD_FILE,BOT_CONFIG_FOLDER,BOT_CONFIG_EXTENSION);

	FILE *fp = CBotGlobals::openFile(buffer,"r");

	CBotMod *curmod = NULL;

	if ( !fp )
	{
		createFile();
		fp = CBotGlobals::openFile(buffer,"r");
	}

	if ( !fp )
	{
		// ERROR!
		return;
	}

	while ( fgets(buffer,1023,fp) != NULL )
	{
		if ( buffer[0] == '#' )
			continue;

		len = strlen(buffer);

		if ( buffer[len-1] == '\n' )
			buffer[--len] = 0;

		i = 0;
		j = 0;

		while ( (i < len) && (buffer[i] != '=') )
		{
			if ( buffer[i] != ' ' )
				key[j++] = buffer[i];
			i++;
		}

		i++;

		key[j] = 0;

		j = 0;

		while ( i < len )
		{
			if ( j || (buffer[i] != ' ') )
				val[j++] = buffer[i];
			i++;
		}

		val[j] = 0;

		if ( !strcmp(key,"mod") )
		{
			if ( curmod )
			{
				curmod->setup(gamefolder,steamfolder,modtype,bottype);
				m_Mods.push_back(curmod);
			}

			curmod = NULL;

			
			bottype = BOTTYPE_GENERIC;

			modtype = MOD_CUSTOM;

			if ( !strcmpi("CUSTOM",val) )
			{
				modtype = MOD_CUSTOM;
				curmod = new CBotMod();
			}
			else if ( !strcmpi("CSS",val) )
			{
				modtype = MOD_CSS;
				curmod = new CCounterStrikeSourceMod();
			}
			else if ( !strcmpi("HL1DM",val) )
			{
				modtype = MOD_HL1DMSRC;
				curmod = new CHLDMSourceMod();
			}
			else if ( !strcmpi("HL2DM",val) )
			{
				modtype = MOD_HLDM2;
				curmod = new CHalfLifeDeathmatchMod();
			}
			else if ( !strcmpi("FF",val) )
			{
				modtype = MOD_FF;
				curmod = new CFortressForeverMod();
			}
			else if ( !strcmpi("TF2",val) )
			{
				modtype = MOD_TF2;
				curmod = new CTeamFortress2Mod();
			}
			else if ( !strcmpi("SVENCOOP2",val) )
			{
				modtype = MOD_SVENCOOP2;
				curmod = new CBotMod();
			}
			else if ( !strcmpi("TIMCOOP",val) )
			{
				modtype = MOD_TIMCOOP;
				curmod = new CBotMod();
			}
			else if ( !strcmpi("NS2",val) )
			{
				modtype = MOD_NS2;
				curmod = new CBotMod();
			}
			else if ( !strcmpi("SYNERGY",val) )
			{
				modtype = MOD_SYNERGY;
				curmod = new CSynergyMod();
			}
			else if ( !strcmpi("DOD",val) )
			{
				modtype = MOD_DOD;
				curmod = new CDODMod();
			}
			else
				curmod = new CBotMod();
		}
		else if ( curmod && !strcmp(key,"bot") )
		{
			if ( !strcmpi("GENERIC",val) )
				bottype = BOTTYPE_GENERIC;
			else if ( !strcmpi("CSS",val) )
				bottype = BOTTYPE_CSS;
			else if ( !strcmpi("HL1DM",val) )
				bottype = BOTTYPE_HL1DM;
			else if ( !strcmpi("HL2DM",val) )
				bottype = BOTTYPE_HL2DM;
			else if ( !strcmpi("FF",val) )
				bottype = BOTTYPE_FF;
			else if ( !strcmpi("TF2",val) )
				bottype = BOTTYPE_TF2;
			else if ( !strcmpi("COOP",val) )
				bottype = BOTTYPE_COOP;
			else if ( !strcmpi("ZOMBIE",val) )
				bottype = BOTTYPE_ZOMBIE;
			else if ( !strcmpi("DOD",val) )
				bottype = BOTTYPE_DOD;
		}
		else if ( curmod && !strcmpi(key,"steamdir") )
		{
			strncpy(steamfolder,val,255);
		}
		else if ( curmod && !strcmpi(key,"gamedir") )
		{
			strncpy(gamefolder,val,255);
		}
	}

	if ( curmod )
	{
		curmod->setup(gamefolder,steamfolder,modtype,bottype);
		m_Mods.push_back(curmod);
	}

	fclose(fp);
}

void CBotMods :: createFile ()
{
	char filename[1024];

	CBotGlobals::buildFileName(filename,BOT_MOD_FILE,BOT_CONFIG_FOLDER,BOT_CONFIG_EXTENSION);

	FILE *fp = CBotGlobals::openFile(filename,"w");

	CBotGlobals::botMessage(NULL,0,"Making a %s.%s file for you... Edit it in '%s'",BOT_MOD_FILE,BOT_CONFIG_EXTENSION,filename);

	if ( fp )
	{
		fprintf(fp,"# EXAMPLE MOD FILE");
		fprintf(fp,"# valid mod types\n");
		fprintf(fp,"# ---------------\n");
		fprintf(fp,"# CSS\n");
		fprintf(fp,"# TF2\n");
		fprintf(fp,"# HL2DM\n");
		fprintf(fp,"# HL1DM\n");
		fprintf(fp,"# FF\n");
		fprintf(fp,"# SVENCOOP2\n");
		fprintf(fp,"# TIMCOOP\n");
		fprintf(fp,"# NS2\n");
		fprintf(fp,"# DOD (day of defeat source)\n");
		fprintf(fp,"#\n");
		fprintf(fp,"# valid bot types\n");
		fprintf(fp,"# ---------------\n");
		fprintf(fp,"# CSS\n");
		fprintf(fp,"# TF2\n");
		fprintf(fp,"# HL2DM\n");
		fprintf(fp,"# HL1DM\n");
		fprintf(fp,"# FF\n");
		fprintf(fp,"# COOP\n");
		fprintf(fp,"# ZOMBIE\n");
		fprintf(fp,"# DOD\n");
		fprintf(fp,"#\n");
		fprintf(fp,"#\n");
		fprintf(fp,"#mod = CSS\n");
		fprintf(fp,"#steamdir = counter-strike source\n");
		fprintf(fp,"#gamedir = cstrike\n");
		fprintf(fp,"#bot = CSS\n");
		fprintf(fp,"#\n");
		fprintf(fp,"#mod = TF2\n");
		fprintf(fp,"#steamdir = teamfortress 2\n");
		fprintf(fp,"#gamedir = tf\n");
		fprintf(fp,"#bot = TF2\n");
		fprintf(fp,"#\n");
		fprintf(fp,"#mod = FF\n");
		fprintf(fp,"#steamdir = sourcemods\n");
		fprintf(fp,"#gamedir = ff\n");
		fprintf(fp,"#bot = FF\n");
		fprintf(fp,"#\n");
		fprintf(fp,"#mod = HL2DM\n");
		fprintf(fp,"#steamdir = half-life 2 deathmatch\n");
		fprintf(fp,"#gamedir = hl2mp\n");
		fprintf(fp,"#bot = HL2DM\n");
		fprintf(fp,"#\n");
		fprintf(fp,"#mod = HL1DM\n");
		fprintf(fp,"#steamdir = half-life 1 deathmatch\n");
		fprintf(fp,"#gamedir = hl1dm\n");
		fprintf(fp,"#bot = HL1DM\n");
		fprintf(fp,"#\n");
		fprintf(fp,"mod = DOD\n");
		fprintf(fp,"steamdir = orangebox\n");
		fprintf(fp,"gamedir = dod\n");
		fprintf(fp,"bot = DOD\n");
		fprintf(fp,"#\n");

		fclose(fp);
	}
	else
		CBotGlobals::botMessage(NULL,0,"Error! Couldn't create config file %s",filename);
}

void CBotMods :: readMods()
{
	m_Mods.push_back(new CDODMod());
	m_Mods.push_back(new CDODModDedicated());

	m_Mods.push_back(new CCounterStrikeSourceMod());
	m_Mods.push_back(new CHalfLifeDeathmatchMod());

	m_Mods.push_back(new CCounterStrikeSourceModDedicated());
	m_Mods.push_back(new CHalfLifeDeathmatchModDedicated());

	m_Mods.push_back(new CFortressForeverMod());
	m_Mods.push_back(new CFortressForeverModDedicated());

	m_Mods.push_back(new CTeamFortress2Mod());
	m_Mods.push_back(new CTeamFortress2ModDedicated());

	m_Mods.push_back(new CHLDMSourceMod());

	// Look for extra MODs

	parseFile();
}

//////////////////////////////////////////////////////////////////////////////

void CBotMod :: setup ( const char *szModFolder, const char *szSteamFolder, eModId iModId, eBotType iBotType )
{
	m_szModFolder = CStrings::getString(szModFolder);
	m_szSteamFolder = CStrings::getString(szSteamFolder);
	m_iModId = iModId;
	m_iBotType = iBotType;
}

/*CBot *CBotMod :: makeNewBots ()
{
	return NULL;
}*/

bool CBotMod :: isSteamFolder ( char *szSteamFolder )
{
	return FStrEq(m_szSteamFolder,szSteamFolder);
}

bool CBotMod :: isModFolder ( char *szModFolder )
{
	return FStrEq(m_szModFolder,szModFolder);
}

char *CBotMod :: getSteamFolder ()
{
	return m_szSteamFolder;
}

char *CBotMod :: getModFolder ()
{
	return m_szModFolder;
}

eModId CBotMod :: getModId ()
{
	return m_iModId;
}

//
// MOD LIST

vector<CBotMod*> CBotMods::m_Mods;

void CBotMods :: freeMemory ()
{
	for ( unsigned int i = 0; i < m_Mods.size(); i ++ )
	{
		m_Mods[i]->freeMemory();
		delete m_Mods[i];
		m_Mods[i] = NULL;
	}

	m_Mods.clear();
}

CBotMod *CBotMods :: getMod ( char *szModFolder, char *szSteamFolder )
{
	for ( unsigned int i = 0; i < m_Mods.size(); i ++ )
	{
		if ( m_Mods[i]->isModFolder(szModFolder) && m_Mods[i]->isSteamFolder(szSteamFolder) )
		{
			return m_Mods[i];
		}
	}

	CBotGlobals::botMessage(NULL,1,"HL2 MODIFICATION \"%s/%s\" NOT FOUND, EXITING... see bot_mods.ini in bot config folder",szSteamFolder,szModFolder);

	return NULL;
}

void CBotMod :: initMod ()
{
	m_bPlayerHasSpawned = false;
}

void CBotMod :: mapInit ()
{
	m_bPlayerHasSpawned = false;
}

bool CBotMod :: playerSpawned ( edict_t *pEntity )
{
	if ( m_bPlayerHasSpawned )
		return false;

	m_bPlayerHasSpawned = true;

	return true;
}

// Naris @ AlliedModders .net

bool CTeamFortress2Mod :: TF2_IsPlayerZoomed(edict_t *pPlayer)
{
    int pcond = CClassInterface :: getTF2Conditions(pPlayer);
    return ((pcond & TF2_PLAYER_ZOOMED) == TF2_PLAYER_ZOOMED);
}

bool CTeamFortress2Mod :: TF2_IsPlayerSlowed(edict_t *pPlayer)
{
    int pcond = CClassInterface :: getTF2Conditions(pPlayer);
    return ((pcond & TF2_PLAYER_SLOWED) == TF2_PLAYER_SLOWED);
}

bool CTeamFortress2Mod :: TF2_IsPlayerDisguised(edict_t *pPlayer)
{
    int pcond = CClassInterface :: getTF2Conditions(pPlayer);
    return ((pcond & TF2_PLAYER_DISGUISED) == TF2_PLAYER_DISGUISED);
}

bool CTeamFortress2Mod :: TF2_IsPlayerTaunting ( edict_t *pPlayer )
{
    int pcond = CClassInterface :: getTF2Conditions(pPlayer);
    return ((pcond & TF2_PLAYER_TAUNTING) == TF2_PLAYER_TAUNTING);
}

bool CTeamFortress2Mod :: TF2_IsPlayerCloaked(edict_t *pPlayer)
{
    int pcond = CClassInterface :: getTF2Conditions(pPlayer);
    return ((pcond & TF2_PLAYER_CLOAKED) == TF2_PLAYER_CLOAKED);
}

bool CTeamFortress2Mod :: TF2_IsPlayerKrits(edict_t *pPlayer)
{
	int pcond = CClassInterface :: getTF2Conditions(pPlayer);
	return ((pcond & TF2_PLAYER_KRITS) == TF2_PLAYER_KRITS);

	return false;
}

bool CTeamFortress2Mod :: TF2_IsPlayerInvuln(edict_t *pPlayer)
{
	if ( CBotGlobals::isPlayer(pPlayer) )
	{
		int pcond = CClassInterface :: getTF2Conditions(pPlayer);
		return ((pcond & TF2_PLAYER_INVULN) == TF2_PLAYER_INVULN);
	}

	return false;
}

bool CTeamFortress2Mod :: TF2_IsPlayerOnFire(edict_t *pPlayer)
{
    int pcond = CClassInterface :: getTF2Conditions(pPlayer);
    return ((pcond & TF2_PLAYER_ONFIRE) == TF2_PLAYER_ONFIRE);
}

int CTeamFortress2Mod ::numClassOnTeam( int iTeam, int iClass )
{
	int i = 0;
	int num = 0;
	edict_t *pEdict;

	for ( i = 1; i <= CBotGlobals::numClients(); i ++ )
	{
		pEdict = INDEXENT(i);

		if ( CBotGlobals::entityIsValid(pEdict) )
		{
			if ( getTeam(pEdict) == iTeam )
			{
				if ( CClassInterface::getTF2Class(pEdict) == iClass )
					num++;
			}
		}
	}

	return num;
}


edict_t *CTeamFortress2Mod :: findResourceEntity()
{
	if ( !m_pResourceEntity )
		m_pResourceEntity = CClassInterface::FindEntityByNetClass(-1, "CTFPlayerResource");

	return m_pResourceEntity;
}

TF_Class CTeamFortress2Mod :: getSpyDisguise ( edict_t *pPlayer )
{
	static int iClass;
	static int iTeam;
	static int iIndex;
	static int iHealth;

	CClassInterface::getTF2SpyDisguised(pPlayer,&iClass,&iTeam,&iIndex,&iHealth);

	return (TF_Class)iClass;
}


float CTeamFortress2Mod :: TF2_GetClassSpeed(int iClass) 
{ 
switch (iClass) 
{ 
case TF_CLASS_SCOUT: return 133.0f; 
case TF_CLASS_SOLDIER: return 80.0f; 
case TF_CLASS_DEMOMAN: return 93.00; 
case TF_CLASS_MEDIC: return 109.0f; 
case TF_CLASS_PYRO: return 100.0f; 
case TF_CLASS_SPY: return 100.0f; 
case TF_CLASS_ENGINEER: return 100.0f; 
case TF_CLASS_SNIPER: return 100.0f; 
case TF_CLASS_HWGUY: return 77.0f; 
} 
return 0.0; 
} 
 
float CTeamFortress2Mod :: TF2_GetPlayerSpeed(edict_t *pPlayer, TF_Class iClass ) 
{ 
	static float fSpeed;

	fSpeed = CClassInterface::getMaxSpeed(pPlayer);// * CClassInterface::getSpeedFactor(pPlayer);

	if ( fSpeed == 0 )
	{
		if (TF2_IsPlayerSlowed(pPlayer)) 
			return 30.0; 
		else 
			return TF2_GetClassSpeed(iClass) * 1.58; 
	}

	return fSpeed;
} 




int CTeamFortress2Mod :: getTeam ( edict_t *pEntity )
{
	return CClassInterface::getTeam (pEntity);
	//return *((int*)(pEntity->GetIServerEntity()->GetBaseEntity())+110);
}

int CTeamFortress2Mod :: getSentryLevel ( edict_t *pSentry )
{
	string_t model = pSentry->GetIServerEntity()->GetModelName();
	const char *szmodel = model.ToCStr();

	return (szmodel[24] - '1')+1;
	//if ( pSentry && pSentry->
}

int CTeamFortress2Mod :: getDispenserLevel ( edict_t *pDispenser )
{
	string_t model = pDispenser->GetIServerEntity()->GetModelName();
	const char *szmodel = model.ToCStr();

	if ( strcmp(szmodel,"models/buildables/dispenser_light.mdl") == 0 )
		return 1;

	return (szmodel[31] - '1')+1;
	//if ( pSentry && pSentry->
}

int CTeamFortress2Mod :: getEnemyTeam ( int iTeam )
{
	if ( iTeam == TF2_TEAM_BLUE )
		return TF2_TEAM_RED;
	return TF2_TEAM_BLUE;
}

/*
------------------int : m_nDisguiseTeam
------------------int : m_nDisguiseClass
------------------int : m_iDisguiseTargetIndex
------------------int : m_iDisguiseHealth

*/

bool CTeamFortress2Mod :: isDispenser ( edict_t *pEntity, int iTeam )
{
	return (!iTeam || (iTeam == getTeam(pEntity))) && (strcmp(pEntity->GetClassName(),"obj_dispenser")==0);
}

bool CTeamFortress2Mod :: isFlag ( edict_t *pEntity, int iTeam )
{
	return (!iTeam || (getEnemyTeam(iTeam) == getTeam(pEntity))) && (strcmp(pEntity->GetClassName(),"item_teamflag")==0);
}

bool CTeamFortress2Mod ::isBoss ( edict_t *pEntity )
{
	if ( m_bBossSummoned )
	{
		if ( m_pBoss.get() && CBotGlobals::entityIsAlive(m_pBoss.get()) )
			return m_pBoss.get() == pEntity;
		else if ( (strcmp(pEntity->GetClassName(),"merasmus")==0)||
			(strcmp(pEntity->GetClassName(),"headless_hatman")==0)||
			(strcmp(pEntity->GetClassName(),"eyeball_boss")==0) )
		{
			m_pBoss = pEntity;
			return true;
		}
	}

	return false;
}

bool CTeamFortress2Mod :: isSentry ( edict_t *pEntity, int iTeam )
{
	return (!iTeam || (iTeam == getTeam(pEntity))) && (strcmp(pEntity->GetClassName(),"obj_sentrygun")==0);
}

bool CTeamFortress2Mod :: isTeleporter ( edict_t *pEntity, int iTeam )
{
	return (!iTeam || (iTeam == getTeam(pEntity))) && (strcmp(pEntity->GetClassName(),"obj_teleporter")==0);
}



bool CTeamFortress2Mod :: isTeleporterEntrance ( edict_t *pEntity, int iTeam )
{
	return isTeleporter(pEntity,iTeam) && CClassInterface::isTeleporterMode(pEntity,TELE_ENTRANCE);
}

bool CTeamFortress2Mod :: isTeleporterExit ( edict_t *pEntity, int iTeam )
{
	return isTeleporter(pEntity,iTeam) && CClassInterface::isTeleporterMode(pEntity,TELE_EXIT);
}

bool CTeamFortress2Mod :: isPipeBomb ( edict_t *pEntity, int iTeam)
{
	return (!iTeam || (iTeam == getTeam(pEntity))) && (strcmp(pEntity->GetClassName(),"tf_projectile_pipe_remote")==0);
}

bool CTeamFortress2Mod :: isRocket ( edict_t *pEntity, int iTeam )
{
	return (!iTeam || (iTeam == getTeam(pEntity))) && (strcmp(pEntity->GetClassName(),"tf_projectile_rocket")==0);
}

bool CHalfLifeDeathmatchMod :: playerSpawned ( edict_t *pPlayer )
{
	if ( CBotMod::playerSpawned(pPlayer) )
	{
		m_LiftWaypoints.clear();

		CWaypoints::updateWaypointPairs(&m_LiftWaypoints,CWaypointTypes::W_FL_LIFT,"func_button");
	}

	return true;
}

void CHalfLifeDeathmatchMod :: initMod ()
{
	unsigned int i;
	// Setup Weapons

	CBots::controlBotSetup(false);

	for ( i = 0; i < HL2DM_WEAPON_MAX; i ++ )
		CWeapons::addWeapon(new CWeapon(HL2DMWeaps[i]));//.iSlot,HL2DMWeaps[i].szWeaponName,HL2DMWeaps[i].iId,HL2DMWeaps[i].m_iFlags,HL2DMWeaps[i].m_iAmmoIndex,HL2DMWeaps[i].minPrimDist,HL2DMWeaps[i].maxPrimDist,HL2DMWeaps[i].m_iPreference,HL2DMWeaps[i].m_fProjSpeed));
}

void CHalfLifeDeathmatchMod :: mapInit ()
{
	CBotMod::mapInit();

	m_LiftWaypoints.clear();
}

void CTeamFortress2Mod :: initMod ()
{
	unsigned int i;
	// Setup Weapons

	CBots::controlBotSetup(true);

	for ( i = 0; i < TF2_WEAPON_MAX; i ++ )
		CWeapons::addWeapon(new CWeapon(TF2Weaps[i]));//.iSlot,TF2Weaps[i].szWeaponName,TF2Weaps[i].iId,TF2Weaps[i].m_iFlags,TF2Weaps[i].m_iAmmoIndex,TF2Weaps[i].minPrimDist,TF2Weaps[i].maxPrimDist,TF2Weaps[i].m_iPreference,TF2Weaps[i].m_fProjSpeed));

	CRCBotTF2UtilFile::loadConfig();
	//memset(g_fBotUtilityPerturb,0,sizeof(float)*TF_CLASS_MAX*BOT_UTIL_MAX);
}


void CTeamFortress2Mod :: mapInit ()
{
	CBotMod::mapInit();

	unsigned int i = 0;
	string_t mapname = gpGlobals->mapname;

	const char *szmapname = mapname.ToCStr();

	m_pResourceEntity = NULL;
	bFlagStateDefault = true;

	if ( strncmp(szmapname,"ctf_",4) == 0 )
		m_MapType = TF_MAP_CTF; // capture the flag
	else if ( strncmp(szmapname,"cp_",3) == 0 )
		m_MapType = TF_MAP_CP; // control point
	else if ( strncmp(szmapname,"tc_",3) == 0 )
		m_MapType = TF_MAP_TC; // territory control
	else if ( strncmp(szmapname,"pl_",3) == 0 )
		m_MapType = TF_MAP_CART; // pipeline
	else if ( strncmp(szmapname,"plr_",4) == 0 )
		m_MapType = TF_MAP_CARTRACE; // pipeline racing
	else if ( strncmp(szmapname,"arena_",6) == 0 )
		m_MapType = TF_MAP_ARENA; // arena mode
	else if ( strncmp(szmapname,"koth_",5) == 0 )
		m_MapType = TF_MAP_KOTH; // king of the hill
	else if ( strncmp(szmapname,"sd_",3) == 0 )
		m_MapType = TF_MAP_SD; // special delivery
	else if ( strncmp(szmapname,"tr_",3) == 0 )
		m_MapType = TF_MAP_TR; // training mode
	else if ( strncmp(szmapname,"mvm_",4) == 0 )
		m_MapType = TF_MAP_MVM; // mann vs machine
	else
		m_MapType = TF_MAP_DM;

	m_iArea = 0;

	m_fSetupTime = 5.0f; // 5 seconds normal

	m_fRoundTime = 0.0f;

	m_pFlagCarrierRed = NULL;
	m_pFlagCarrierBlue = NULL;
	m_iFlagCarrierTeam = 0;

	for ( i = 0; i < MAX_PLAYERS; i ++ )
	{
		m_Teleporters[i].entrance = MyEHandle(NULL);
		m_Teleporters[i].exit = MyEHandle(NULL);
		m_Teleporters[i].sapper = MyEHandle(NULL);
		m_SentryGuns[i].sapper = MyEHandle(NULL);
		m_SentryGuns[i].sentry = MyEHandle(NULL);
		m_Dispensers[i].sapper = MyEHandle(NULL);
		m_Dispensers[i].disp = MyEHandle(NULL);
		pMediGuns[i] = NULL;
	}

	m_bAttackDefendMap = false;
	m_pBoss = NULL;
	m_bBossSummoned = false;

	resetCappers();
	CPoints::loadMapScript();

}

bool CDODMod :: shouldAttack ( int iTeam )
// uses the perceptron to return probability of attack
{
	static short int iFlags_0;
	static short int iFlags_1;
	static short int iNumFlags;


	iNumFlags = m_Flags.getNumFlags();

	iFlags_0 = (int) (((float)m_Flags.getNumFlagsOwned(iTeam == TEAM_ALLIES ? TEAM_AXIS : TEAM_ALLIES) / iNumFlags)*MAX_DOD_FLAGS);
	iFlags_1 = (int) (((float)m_Flags.getNumFlagsOwned(iTeam) / iNumFlags)*MAX_DOD_FLAGS);

	return randomFloat(0.0,1.0) < fAttackProbLookUp[iFlags_0][iFlags_1];//gNetAttackOrDefend->getOutput();
}
////////////////////////////////////////////////
void CDODMod :: initMod ()
{
	unsigned int i;

	// Setup Weapons

///-------------------------------------------------
	CBotGlobals::botMessage(NULL,0,"Training DOD:S capture decision 'NN' ... hold on...");

	CBotNeuralNet *nn = new CBotNeuralNet(2,2,2,1,0.4f);

	CTrainingSet *tset = new CTrainingSet(2,1,4);

	tset->setScale(0.0,1.0);

	tset->addSet();
	tset->in(1.0/5); // E - enemy flag ratio
	tset->in(1.0/5); // T - team flag ratio
	tset->out(0.9f); // probability of attack

	tset->addSet();
	tset->in(4.0/5); // E - enemy flag ratio
	tset->in(1.0/5); // T - team flag ratio
	tset->out(0.2f); // probability of attack (mostly defend)
	
	tset->addSet();
	tset->in(1.0/5); // E - enemy flag ratio
	tset->in(4.0/5); // T - team flag ratio
	tset->out(0.9f); // probability of attack

	tset->addSet();
	tset->in(0.5f); // E - enemy flag ratio
	tset->in(0.5f); // T - team flag ratio
	tset->out(0.6f); // probability of attack

	nn->batch_train(tset,1000);

	// create look up table for probabilities
	for ( short int i = 0; i <= MAX_DOD_FLAGS; i ++ )
	{
		for ( short int j = 0; j <= MAX_DOD_FLAGS; j ++ )
		{
			tset->init();
			tset->addSet();
			tset->in(((float)i) / MAX_DOD_FLAGS);
			tset->in(((float)j) / MAX_DOD_FLAGS);
			nn->execute(tset->getBatches()->in,&(fAttackProbLookUp[i][j]),0.0f,1.0f);
		}
	}

	tset->freeMemory();
	delete tset;
	delete nn;

	CBotGlobals::botMessage(NULL,0,"... done!");
///-------------------------------------------------


	CBots::controlBotSetup(true);

	for ( i = 0; i < DOD_WEAPON_MAX; i ++ )
		CWeapons::addWeapon(new CWeapon(DODWeaps[i]));

	m_pResourceEntity = NULL;
}

void CDODMod :: mapInit ()
{
	CBotMod::mapInit();

	m_pResourceEntity = NULL;
	m_pGameRules = NULL;
	m_pPlayerResourceEntity = NULL;
	m_Flags.init();
	m_fMapStartTime = engine->Time();
	m_iMapType = DOD_MAPTYPE_UNKNOWN;
	m_bCommunalBombPoint = false;
}


float CDODMod::getMapStartTime () 
{ 
	//if ( !m_pGameRules ) 
	//	return 0; 
	return m_fMapStartTime;
	//return CClassInterface::getRoundTime(m_pGameRules); 
}

int CDODMod::getHighestScore ()
{
	if ( !m_pPlayerResourceEntity )
		return 0;

	int highest = 0;
	int score;
	short int i = 0;
	edict_t *edict;

	for ( i = 1; i <= gpGlobals->maxClients; i ++ )
	{
		edict = INDEXENT(i);

		if ( edict && CBotGlobals::entityIsValid(edict) )
		{
			score = (short int)getScore(edict);
		
			if ( score > highest )
			{
				highest = score;
			}
		}
	}

	return highest;
}

bool CDODFlags::isTeamMateDefusing ( edict_t *pIgnore, int iTeam, int id )
{
	if ( m_pBombs[id][0] != NULL )
		return isTeamMateDefusing(pIgnore,iTeam,CBotGlobals::entityOrigin(m_pBombs[id][0]));

	return false;
}

bool CDODFlags::isTeamMateDefusing ( edict_t *pIgnore, int iTeam, Vector vOrigin )
{
	int i;
	edict_t *pPlayer;
	IPlayerInfo *pPlayerinfo;
	CBotCmd cmd;

	for ( i = 1; i <= gpGlobals->maxClients; i ++ )
	{
		pPlayer = INDEXENT(i);

		if ( pIgnore == pPlayer )
			continue;

		if ( !CBotGlobals::entityIsValid(pPlayer) )
			continue;

		pPlayerinfo = playerinfomanager->GetPlayerInfo(pPlayer);
		cmd = pPlayerinfo->GetLastUserCommand();

		if ( CClassInterface::isPlayerDefusingBomb_DOD(pPlayer) || (cmd.buttons & IN_USE) )
		{
			if ( CClassInterface::getTeam(pPlayer) != iTeam )
				continue;

			if ( (vOrigin - CBotGlobals::entityOrigin(pPlayer)).Length() < 128 )
			{
				return true;
			}
		}
	}

	return false;
}


bool CDODFlags::isTeamMatePlanting ( edict_t *pIgnore, int iTeam, Vector vOrigin )
{
	int i;
	edict_t *pPlayer;

	for ( i = 1; i <= gpGlobals->maxClients; i ++ )
	{
		pPlayer = INDEXENT(i);

		if ( pIgnore == pPlayer )
			continue;

		if ( !CBotGlobals::entityIsValid(pPlayer) )
			continue;

		if ( CClassInterface::isPlayerPlantingBomb_DOD(pPlayer) )
		{
			if ( CClassInterface::getTeam(pPlayer) != iTeam )
				continue;

			if ( (vOrigin - CBotGlobals::entityOrigin(pPlayer)).Length() < 128 )
			{
				return true;
			}
		}
	}

	return false;
}

bool CDODFlags::isTeamMatePlanting ( edict_t *pIgnore, int iTeam, int id )
{
	if ( m_pBombs[id][0] )
		return isTeamMatePlanting(pIgnore,iTeam,CBotGlobals::entityOrigin(m_pBombs[id][0]));

	return false;
}
// return the flag with the least danger (randomly)
bool CDODFlags::getRandomEnemyControlledFlag ( CBot *pBot, Vector *position, int iTeam, int *id )
{
	IBotNavigator *pNav;
	float fTotal;
	float fRand;

	if ( id )
		*id = -1;

	pNav = pBot->getNavigator();

	fTotal = 0.0f;

	for ( short int i = 0; i < m_iNumControlPoints; i ++ )
	{
		if ( m_iWaypoint[i] != -1 )
		{
			if ( ( m_pFlags[i] == NULL ) || ( m_iOwner[i] == iTeam ) )
				continue;

			if ( (iTeam == TEAM_ALLIES) && (m_iAlliesReqCappers[i] == 0) )
				continue;

			if ( (iTeam == TEAM_AXIS) && (m_iAxisReqCappers[i] == 0) )
				continue;

			if ( iTeam == TEAM_ALLIES )
				fTotal += (((MAX_BELIEF + 1.0f) - pNav->getBelief(m_iWaypoint[i])) / MAX_BELIEF) * (m_iNumAllies[i]+1);
			else
				fTotal += (((MAX_BELIEF + 1.0f) - pNav->getBelief(m_iWaypoint[i])) / MAX_BELIEF) * (m_iNumAxis[i]+1);
		}
	}

	if ( fTotal == 0.0f )
		return false;

	fRand = randomFloat(0,fTotal);
	fTotal = 0.0f;

	for ( short int i = 0; i < m_iNumControlPoints; i ++ )
	{
		if ( m_iWaypoint[i] != -1 )
		{
			if ( ( m_pFlags[i] == NULL ) || ( m_iOwner[i] == iTeam ) )
				continue;

			if ( (iTeam == TEAM_ALLIES) && (m_iAlliesReqCappers[i] == 0) )
				continue;

			if ( (iTeam == TEAM_AXIS) && (m_iAxisReqCappers[i] == 0) )
				continue;

			if ( iTeam == TEAM_ALLIES )
				fTotal += (((MAX_BELIEF + 1.0f) - pNav->getBelief(m_iWaypoint[i])) / MAX_BELIEF) * (m_iNumAllies[i]+1);
			else
				fTotal += (((MAX_BELIEF + 1.0f) - pNav->getBelief(m_iWaypoint[i])) / MAX_BELIEF) * (m_iNumAxis[i]+1);
		}

		if ( fRand <= fTotal )
		{
			if ( id )
				*id = i;
			*position = m_vCPPositions[i];
			return true;
		}
	}

	return false;
}

bool CDODFlags::getRandomBombToDefuse  ( Vector *position, int iTeam, edict_t **pBombTarget, int *id )
{
	vector<int> iPossible;
	short int j;
	int selection;

	if ( id )
		*id = -1;

	// more possibility to return bomb targets with no bomb already
	for ( short int i = 0; i < m_iNumControlPoints; i ++ )
	{
		if ( (m_iOwner[i] == iTeam) && isBombPlanted(i) && !isBombBeingDefused(i) && (m_pBombs[i][0] != NULL) )
			for ( j = 0; j < getNumBombsRequired(i); j ++ ) { iPossible.push_back(i); }
	}

	if ( iPossible.size() > 0 )
	{
		selection = iPossible[randomInt(0,iPossible.size()-1)];

		if ( m_pBombs[selection][1] != NULL )
		{
			if ( CClassInterface::getDODBombState(m_pBombs[selection][1]) == DOD_BOMB_STATE_ACTIVE )
				*pBombTarget = m_pBombs[selection][1];
			else
				*pBombTarget = m_pBombs[selection][0];
		}
		else
			*pBombTarget = m_pBombs[selection][0];

		*position = CBotGlobals::entityOrigin(*pBombTarget);

		if ( id ) // area of the capture point
			*id = selection;
	}

	return (iPossible.size()>0);
}

//return random bomb with highest danger
bool CDODFlags:: getRandomBombToDefend ( CBot *pBot, Vector *position, int iTeam, edict_t **pBombTarget, int *id )
{
	vector<int> iPossible;
	short int j;
	int selection;

	if ( id )
		*id = -1;

	// more possibility to return bomb targets with no bomb already
	for ( short int i = 0; i < m_iNumControlPoints; i ++ )
	{
		if ( (m_iOwner[i] != iTeam) && isBombPlanted(i) && (m_pBombs[i][0] != NULL) )
			for ( j = 0; j < getNumBombsRequired(i); j ++ ) { iPossible.push_back(i); }
	}

	if ( iPossible.size() > 0 )
	{
		selection = iPossible[randomInt(0,iPossible.size()-1)];

		if ( m_pBombs[selection][1] != NULL )
		{
			if ( CClassInterface::getDODBombState(m_pBombs[selection][1]) != 0 )
				*pBombTarget = m_pBombs[selection][1];
			else
				*pBombTarget = m_pBombs[selection][0];
		}
		else
			*pBombTarget = m_pBombs[selection][0];

		*position = CBotGlobals::entityOrigin(*pBombTarget);

		if ( id ) // area of the capture point
			*id = selection;
	}

	return (iPossible.size()>0);
}

// return rnaomd flag with lowest danger
bool CDODFlags:: getRandomBombToPlant ( CBot *pBot, Vector *position, int iTeam, edict_t **pBombTarget, int *id )
{
	float fTotal;
	float fRand;

	IBotNavigator *pNav;

//	short int j;
	int selection;

	if ( id )
		*id = -1;

	selection = -1;

	pNav = pBot->getNavigator();

	fTotal = 0.0f;

	for ( short int i = 0; i < m_iNumControlPoints; i ++ )
	{
		// if no waypoint -- can't go there
		if ( m_iWaypoint[i] != -1 )
		{
			if ( ( m_pBombs[i][0] == NULL ) || ( m_iOwner[i] == iTeam ) || isBombPlanted(i) || (m_iBombsRemaining[i] == 0) )
				continue;

			fTotal += (((MAX_BELIEF + 1.0f) - pNav->getBelief(m_iWaypoint[i])) / MAX_BELIEF) * getNumBombsRemaining(i);
		}
	}

	if ( fTotal == 0.0f )
		return false;

	fRand = randomFloat(0.0f,fTotal);

	fTotal = 0.0f;

	for ( short int i = 0; i < m_iNumControlPoints; i ++ )
	{
		if ( m_iWaypoint[i] != -1 )
		{
			if ( ( m_pBombs[i][0] == NULL ) || ( m_iOwner[i] == iTeam ) || isBombPlanted(i) )
				continue;

				fTotal += (((MAX_BELIEF + 1.0f) - pNav->getBelief(m_iWaypoint[i])) / MAX_BELIEF) * getNumBombsRemaining(i);
		}
		else
			fTotal += 0.1f;

		if ( fRand <= fTotal )
		{
			selection = i;

			if ( m_pBombs[selection][1] != NULL )
			{
				if ( CClassInterface::getDODBombState(m_pBombs[selection][1]) == DOD_BOMB_STATE_AVAILABLE )
					*pBombTarget = m_pBombs[selection][1];
				else
					*pBombTarget = m_pBombs[selection][0];
			}
			else
				*pBombTarget = m_pBombs[selection][0];

			*position = CBotGlobals::entityOrigin(*pBombTarget);

			if ( id ) // area of the capture point
				*id = selection;

			return true;
		}
	}

	return false;
}


bool CDODFlags::getRandomTeamControlledFlag ( CBot *pBot, Vector *position, int iTeam, int *id )
{
	IBotNavigator *pNav;
	float fTotal;
	float fRand;

	if ( id )
		*id = -1;

	pNav = pBot->getNavigator();

	fTotal = 0.0f;

	for ( short int i = 0; i < m_iNumControlPoints; i ++ )
	{
		if ( m_iWaypoint[i] != -1 )
		{
			if ( ( m_pFlags[i] == NULL ) || ( m_iOwner[i] != iTeam ) )
				continue;

			if ( iTeam == TEAM_AXIS )
				fTotal += ((pNav->getBelief(m_iWaypoint[i])+MAX_BELIEF)/(MAX_BELIEF*2)) * (m_iNumAllies[i]+1);
			else
				fTotal += ((pNav->getBelief(m_iWaypoint[i])+MAX_BELIEF)/(MAX_BELIEF*2)) * (m_iNumAxis[i]+1);
		}
	}

	if ( fTotal == 0.0f )
		return false;

	fRand = randomFloat(0,fTotal);
	fTotal = 0.0f;

	for ( short int i = 0; i < m_iNumControlPoints; i ++ )
	{
		if ( m_iWaypoint[i] != -1 )
		{
			if ( ( m_pFlags[i] == NULL ) || ( m_iOwner[i] != iTeam ) )
				continue;

			if ( iTeam == TEAM_AXIS )
				fTotal += ((pNav->getBelief(m_iWaypoint[i])+MAX_BELIEF)/(MAX_BELIEF*2)) * (m_iNumAllies[i]+1);
			else
				fTotal += ((pNav->getBelief(m_iWaypoint[i])+MAX_BELIEF)/(MAX_BELIEF*2)) * (m_iNumAxis[i]+1);
		}

		if ( fRand <= fTotal )
		{
			if ( id )
				*id = i;
			*position = m_vCPPositions[i];
			return true;
		}
	}

	return false;
}

void CDODMod::freeMemory()
{

}

// returns map type
int CDODFlags::setup(edict_t *pResourceEntity)
{
	int iNumBombCaps = 0;
	int iNumFlags = 0;
	//bool *CPsVisible;

	m_iNumControlPoints = 0;

	memset(m_bBombPlanted,0,sizeof(bool)*MAX_DOD_FLAGS); // all false

	//CPsVisible = CClassInterface::getDODCPVisible(pResourceEntity);

	if ( pResourceEntity )
	{  
		// get the arrays from the resource entity
		CClassInterface::getDODFlagInfo(pResourceEntity,&m_iNumAxis,&m_iNumAllies,&m_iOwner,&m_iAlliesReqCappers,&m_iAxisReqCappers);
		CClassInterface::getDODBombInfo(pResourceEntity,&m_bBombPlanted_Unreliable,&m_iBombsRequired,&m_iBombsRemaining,&m_bBombBeingDefused);
		m_iNumControlPoints = CClassInterface::getDODNumControlPoints(pResourceEntity);
		// get the Capture point positions
		m_vCPPositions = CClassInterface::getDODCP_Positions(pResourceEntity);

	}

	short int i,j;

//	string_t model;		
//	const char *modelname;
//	bool bVisible;
				

	// find the edicts of the flags using the origin and classname

	for ( j = 0; j < m_iNumControlPoints; j ++ )
	{
		edict_t *pent;

		Vector vOrigin;

		i = gpGlobals->maxClients;

		// find visible flags -- with a model
		while ( (++i < gpGlobals->maxEntities) &&  (m_pFlags[j] == NULL ) )
		{
			pent = INDEXENT(i);

			if ( !pent || pent->IsFree() )
				continue;

			if ( strcmp(pent->GetClassName(),DOD_CLASSNAME_CONTROLPOINT) == 0 )
			{
				vOrigin = CBotGlobals::entityOrigin(pent);

				if ( vOrigin == m_vCPPositions[j] )
				{
					/*
					bVisible = ((CClassInterface::getEffects(pent) & EF_NODRAW) != EF_NODRAW);

					model = pent->GetIServerEntity()->GetModelName();
					modelname = model.ToCStr();

					if ( bVisible && modelname && *modelname )
					{*/

					if ( m_iAlliesReqCappers[j] || m_iAxisReqCappers[j] || m_iBombsRequired[j] )
					{
						m_pFlags[j] = pent;
					}

					break; // found it
				}
			}
		}

		// no flag for this point
		if ( m_pFlags[j] == NULL ) 
			continue;

		// don't need to check for bombs
		if ( m_iBombsRequired[j] == 0 )
			continue;

		// find bombs near flag
		i = gpGlobals->maxClients;

		while ( (++i < gpGlobals->maxEntities) && ((m_pBombs[j][0]==NULL)||(m_pBombs[j][1]==NULL)) )
		{
			pent = INDEXENT(i);

			if ( !pent || pent->IsFree() )
				continue;

			if ( strcmp(pent->GetClassName(),DOD_CLASSNAME_BOMBTARGET) == 0 )
			{
				vOrigin = CBotGlobals::entityOrigin(pent);

				if ( (vOrigin - m_vCPPositions[j]).Length() < 400.0f )
				{
					if ( m_pBombs[j][0] == NULL )
					{
						m_pBombs[j][0] = pent;						
					}
					else
						m_pBombs[j][1] = pent;
				}
			}
		}
	}

	// find waypoints
	for ( short int i = 0; i < m_iNumControlPoints; i ++ )
	{
		// if we don't know any cap waypoint yet here find one
		if ( m_iWaypoint[i] == -1 )
		{
			// get any nearby waypoint so the bot knows which waypoint to get danger from
			// look for the nearest waypoint which is a cap point
			m_iWaypoint[i] = CWaypointLocations::NearestWaypoint(m_vCPPositions[i],400.0f,-1,false,false,false,0,false,0,false,false,Vector(0,0,0),CWaypointTypes::W_FL_CAPPOINT );

			// still no waypoint, search for any capture waypoint with the same area
			if ( m_iWaypoint[i] == -1 )
			{
				m_iWaypoint[i] = CWaypoints::getWaypointIndex(CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_CAPPOINT,0,i,true));
			}
		}
	}

	m_iNumAxisBombsOnMap = getNumPlantableBombs(TEAM_AXIS);
	m_iNumAlliesBombsOnMap = getNumPlantableBombs(TEAM_ALLIES);

	// sometimes m_iNumControlPoints is larger than it  should be. check the number of flags and bombs we found on the map
	for ( short int i = 0; i < m_iNumControlPoints; i ++ )
	{
		if ( m_pFlags[i] != NULL )
			iNumFlags++;
		if ( m_pBombs[i][0] != NULL )
			iNumBombCaps++;
		if ( m_pBombs[i][1] != NULL )
			iNumBombCaps++;
	}

	// update new number
	m_iNumControlPoints = iNumFlags;

	if ( iNumBombCaps >= iNumFlags )
		return DOD_MAPTYPE_BOMB;

	return DOD_MAPTYPE_FLAG;
}

int CDODMod ::getScore(edict_t *pPlayer)
{
	if ( m_pPlayerResourceEntity )
		return CClassInterface::getPlayerScoreDOD(m_pPlayerResourceEntity,pPlayer) + 
		CClassInterface::getPlayerObjectiveScoreDOD(m_pPlayerResourceEntity,pPlayer) - 
		CClassInterface::getPlayerDeathsDOD(pPlayer,m_pPlayerResourceEntity);

	return 0;
}

edict_t *CDODMod :: getBreakable ( CWaypoint *pWpt )
{
	register unsigned short int size = m_BreakableWaypoints.size();

	for ( register unsigned short int i = 0; i < size; i ++ )
	{
		if ( m_BreakableWaypoints[i].pWaypoint == pWpt )
			return m_BreakableWaypoints[i].pEdict;
	}

	return NULL;
}

edict_t *CDODMod :: getBombTarget ( CWaypoint *pWpt )
{
	register unsigned short int size = m_BombWaypoints.size();

	for ( register unsigned short int i = 0; i < size; i ++ )
	{
		if ( m_BombWaypoints[i].pWaypoint == pWpt )
			return m_BombWaypoints[i].pEdict;
	}

	return NULL;
}

void CDODMod ::roundStart()
{
	if ( !m_pResourceEntity )
		m_pResourceEntity = CClassInterface::FindEntityByNetClass(gpGlobals->maxClients+1, "CDODObjectiveResource");
	if ( !m_pPlayerResourceEntity )
		m_pPlayerResourceEntity = CClassInterface::FindEntityByNetClass(gpGlobals->maxClients+1, "CDODPlayerResource");
	if ( !m_pGameRules )
		m_pGameRules = CClassInterface::FindEntityByNetClass(gpGlobals->maxClients+1, "CDODGameRulesProxy");

	// find main map type
	m_iMapType = m_Flags.setup(m_pResourceEntity);

	//if ( m_iMapType == DOD_MAPTYPE_UNKNOWN )
	//{
		if ( CClassInterface::FindEntityByNetClass(gpGlobals->maxClients+1,"CDODBombDispenserMapIcon") != NULL )
		{
			CWaypoint *pWaypointAllies;
			CWaypoint *pWaypointAxis;

			// add bitmask
			m_iMapType |= DOD_MAPTYPE_BOMB;
/*
			if ( m_iMapType == DOD_MAPTYPE_FLAG) 
				CRCBotPlugin::HudTextMessage(CClients::get(0)->getPlayer(),"RCBot detected Flag map","RCBot2","RCbot2 detected a flag map");
			else if ( m_iMapType == DOD_MAPTYPE_BOMB )
				CRCBotPlugin::HudTextMessage(CClients::get(0)->getPlayer(),"RCBot detected bomb map","RCBot2","RCbot2 detected a bomb map");
			else if ( m_iMapType == 3 )
				CRCBotPlugin::HudTextMessage(CClients::get(0)->getPlayer(),"RCBot detected flag map with bombs ","RCBot2","RCbot2 detected a flag capture map with bombs");

*/
			pWaypointAllies = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_BOMBS_HERE,TEAM_ALLIES);
			pWaypointAxis = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_BOMBS_HERE,TEAM_AXIS);

			if ( pWaypointAllies && pWaypointAxis )
			{
				m_bCommunalBombPoint = (pWaypointAllies->getArea()>0) || (pWaypointAxis->getArea()>0);

				m_iBombAreaAllies = pWaypointAllies->getArea();
				m_iBombAreaAxis = pWaypointAxis->getArea();
			}
		}
		//else
		//	m_iMapType = DOD_MAPTYPE_FLAG;
	//}

	// find bombs at waypoints
	m_BombWaypoints.clear();
	m_BreakableWaypoints.clear();

	CWaypoints::updateWaypointPairs(&m_BombWaypoints,CWaypointTypes::W_FL_BOMB_TO_OPEN,"dod_bomb_target");
	CWaypoints::updateWaypointPairs(&m_BreakableWaypoints,CWaypointTypes::W_FL_BREAKABLE,"func_breakable");

	//m_Flags.updateAll();
}
// when a bomb explodes it might leave a part of the ground available
// find it and add it as a waypoint offset
Vector CDODMod :: getGround ( CWaypoint *pWaypoint )
{
	for ( unsigned int i = 0; i < m_BombWaypoints.size(); i ++ )
	{
		if ( m_BombWaypoints[i].pWaypoint == pWaypoint )
		{
			if ( m_BombWaypoints[i].pEdict )
			{
				if ( CClassInterface::getDODBombState(m_BombWaypoints[i].pEdict) == 0 )
					return m_BombWaypoints[i].v_ground;
				
				break;
			}
		}
	}

	return pWaypoint->getOrigin();
}

void CDODMod :: addWaypointFlags (edict_t *pPlayer, edict_t *pEdict, int *iFlags, int *iArea, float *fMaxDistance )
{
	if ( isBombMap()  )
	{
		int id = m_Flags.getBombID(pEdict);

		if ( id != -1 )
		{
			*iFlags |= CWaypointTypes::W_FL_CAPPOINT;
			*iArea = id;
		}
	}
	
	if ( isFlagMap() )
	{
		int id = m_Flags.getFlagID(pEdict);

		if ( id != -1 )
		{
			*iFlags |= CWaypointTypes::W_FL_CAPPOINT;
			*iArea = id;
		}
	}

}

void CDODMod :: modFrame()
{

}


int CDODMod ::numClassOnTeam( int iTeam, int iClass )
{
	int i = 0;
	int num = 0;
	edict_t *pEdict;

	for ( i = 1; i <= CBotGlobals::numClients(); i ++ )
	{
		pEdict = INDEXENT(i);

		if ( CBotGlobals::entityIsValid(pEdict) )
		{
			if ( CClassInterface::getTeam(pEdict) == iTeam )
			{
				if ( CClassInterface::getPlayerClassDOD(pEdict) == iClass )
					num++;
			}
		}
	}

	return num;
}

void CDODMod ::clientCommand( edict_t *pEntity, int argc, const char *pcmd, const char *arg1, const char *arg2 )
{
	if ( argc == 1 )
	{
		if ( strncmp(pcmd,"voice_",6) == 0 )
		{
			short int i;
			// somebody said a voice command
			u_VOICECMD vcmd;

			for ( i = 0; i < DOD_VC_INVALID; i ++ )
			{
				if ( strcmp(&pcmd[6],g_DODVoiceCommands[i].pcmd) == 0 )
				{
					vcmd.voicecmd = i;

					CBroadcastVoiceCommand voicecmd = CBroadcastVoiceCommand(pEntity,vcmd.voicecmd); 

					CBots::botFunction(&voicecmd);

					break;
				}
			}
		}
	}
}

bool CDODMod :: isBreakableRegistered ( edict_t *pBreakable, int iTeam )
{
	static CWaypoint *pWpt;

	for ( unsigned int i = 0; i < m_BreakableWaypoints.size(); i ++ )
	{
		if ( m_BreakableWaypoints[i].pEdict == pBreakable )
		{
			pWpt = m_BreakableWaypoints[i].pWaypoint;

			if ( pWpt->hasFlag(CWaypointTypes::W_FL_NOALLIES) )
				return iTeam != TEAM_ALLIES;
			else if ( pWpt->hasFlag(CWaypointTypes::W_FL_NOAXIS) )
				return iTeam != TEAM_AXIS;

			return true;
		}
	}

	return false;
}

void CTeamFortress2Mod :: getTeamOnlyWaypointFlags ( int iTeam, int *iOn, int *iOff )
{
	if ( iTeam == TF2_TEAM_BLUE )
	{
		*iOn = CWaypointTypes::W_FL_NORED;
		*iOff = CWaypointTypes::W_FL_NOBLU;
	}
	else if ( iTeam == TF2_TEAM_RED )
	{
		*iOn = CWaypointTypes::W_FL_NOBLU;
		*iOff = CWaypointTypes::W_FL_NORED;
	}

}

void CDODMod :: getTeamOnlyWaypointFlags ( int iTeam, int *iOn, int *iOff )
{
	if ( iTeam == TEAM_ALLIES )
	{
		*iOn = CWaypointTypes::W_FL_NOAXIS;
		*iOff = CWaypointTypes::W_FL_NOALLIES;
	}
	else if ( iTeam == TEAM_AXIS )
	{
		*iOn = CWaypointTypes::W_FL_NOALLIES;
		*iOff = CWaypointTypes::W_FL_NOAXIS;
	}


}
