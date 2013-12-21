/*
 *    part of https://rcbot2.svn.sourceforge.net/svnroot/rcbot2
 *
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

#include "bot.h"
#include "bot_fortress.h"
#include "bot_buttons.h"
#include "bot_globals.h"
#include "bot_profile.h"
#include "bot_schedule.h"
#include "bot_task.h"
#include "bot_waypoint.h"
#include "bot_navigator.h"
#include "bot_mods.h"
#include "bot_visibles.h"
#include "bot_weapons.h"
#include "bot_waypoint_locations.h"
#include "in_buttons.h"
#include "bot_utility.h"
#include "bot_script.h"
#include "bot_configfile.h"
#include "bot_getprop.h"
#include "bot_mtrand.h"

extern ConVar bot_beliefmulti;
extern ConVar bot_spyknifefov;
extern ConVar bot_use_vc_commands;
extern ConVar bot_max_cc_time;
extern ConVar bot_min_cc_time;
extern ConVar bot_change_class;
extern ConVar rcbot_demo_jump;
extern ConVar rcbot_melee_only;
extern ConVar rcbot_tf2_protect_cap_time;
extern ConVar rcbot_tf2_protect_cap_percent;
extern ConVar rcbot_tf2_spy_kill_on_cap_dist;
extern ConVar rcbot_speed_boost;
extern ConVar rcbot_projectile_tweak;

#define TF2_SPY_CLOAK_BELIEF 38
#define TF2_HWGUY_REV_BELIEF 60
//extern float g_fBotUtilityPerturb [TF_CLASS_MAX][BOT_UTIL_MAX];

// Payload stuff by   The_Shadow

//#include "vstdlib/random.h" // for random functions

void CBroadcastOvertime ::execute (CBot*pBot)
{
	pBot->updateCondition(CONDITION_PUSH);
}
void CBroadcastCapturedPoint :: execute ( CBot *pBot )
{
	((CBotTF2*)pBot)->pointCaptured(m_iPoint,m_iTeam,m_szName);
}

CBroadcastCapturedPoint :: CBroadcastCapturedPoint ( int iPoint, int iTeam, const char *szName )
{
	m_iPoint = iPoint;
	m_iTeam = iTeam;
	m_szName = CStrings::getString(szName);
}

void CBroadcastSpySap :: execute ( CBot *pBot )
{
	if ( CTeamFortress2Mod::getTeam(m_pSpy) != pBot->getTeam() )
	{
		if ( pBot->isVisible(m_pSpy) )
			((CBotTF2*)pBot)->foundSpy(m_pSpy,CTeamFortress2Mod::getSpyDisguise(m_pSpy));
	}
}
// special delivery
void CBroadcastFlagReturned :: execute ( CBot*pBot )
{
	//if ( pBot->getTeam() == m_iTeam )
		//((CBotTF2*)pBot)->flagReturned_SD(m_vOrigin);
	//else
	//	((CBotTF2*)pBot)->teamFlagDropped(m_vOrigin);	
}

void CBroadcastFlagDropped :: execute ( CBot *pBot )
{
	if ( pBot->getTeam() == m_iTeam )
		((CBotTF2*)pBot)->flagDropped(m_vOrigin);
	else
		((CBotTF2*)pBot)->teamFlagDropped(m_vOrigin);
}
// flag picked up
void CBotTF2FunctionEnemyAtIntel :: execute (CBot *pBot)
{
	pBot->updateCondition(CONDITION_PUSH);

	if ( pBot->getTeam() != m_iTeam )
	{
		((CBotTF2*)pBot)->enemyAtIntel(m_vPos,m_iType);
	}
	else
		((CBotTF2*)pBot)->teamFlagPickup();
}

void CBotTF2 :: hearVoiceCommand ( edict_t *pPlayer, byte cmd )
{
	switch ( cmd )
	{
	case TF_VC_SPY:
		// someone shouted spy, HACK the bot to think they saw a spy here too
		// for spy checking purposes
		if ( isVisible(pPlayer) )
		{
			m_vLastSeeSpy = CBotGlobals::entityOrigin(pPlayer);
			m_fSeeSpyTime = engine->Time() + randomFloat(3.0f,6.0f);
			//m_pPrevSpy = pPlayer; // HACK
		}
		break;
	// somebody shouted "MEDIC!"
	case TF_VC_MEDIC:
		medicCalled(pPlayer);
		break;
	case TF_VC_SENTRYHERE: // hear 'put sentry here' 
		// if I'm carrying a sentry just drop it here
		if ( getClass() == TF_CLASS_ENGINEER )
		{
			if ( m_bIsCarryingObj && m_bIsCarryingSentry )
			{
				if ( isVisible(pPlayer) && (distanceFrom(pPlayer) < 512) )
				{
					if ( randomInt(0,100) > 75 )
						addVoiceCommand(TF_VC_YES);

					primaryAttack();

					m_pSchedules->removeSchedule(SCHED_TF2_ENGI_MOVE_BUILDING);
				}
				else if ( randomInt(0,100) > 75 )
					addVoiceCommand(TF_VC_NO);
			}
		}
		break;
	case TF_VC_HELP:
		// add utility can find player
		if ( isVisible(pPlayer) )
		{
			if ( !m_pSchedules->isCurrentSchedule(SCHED_GOTO_ORIGIN) )
			{
				m_pSchedules->removeSchedule(SCHED_GOTO_ORIGIN);

				m_pSchedules->addFront(new CBotGotoOriginSched(pPlayer));
			}
		}
		break;
	case TF_VC_GOGOGO:
		// if bot is nesting, or waiting for something, it will go
		if ( distanceFrom(pPlayer) > 512 )
			return;

		updateCondition(CONDITION_PUSH);

		if ( randomFloat(0,1.0) > 0.75f )
			m_nextVoicecmd = TF_VC_YES;

		// don't break // flow down to uber if medic
	case TF_VC_ACTIVATEUBER:
		if ( CTeamFortress2Mod::hasRoundStarted() && (getClass() == TF_CLASS_MEDIC)  )
		{
			if ( m_pHeal == pPlayer )
			{
				if ( !CTeamFortress2Mod::isFlagCarrier(pPlayer) )
					secondaryAttack();
				else if ( randomFloat(0,1.0) > 0.5f )
					m_nextVoicecmd = TF_VC_NO;
			}
		}
		break;
	case TF_VC_MOVEUP:

		if ( distanceFrom(pPlayer) > 1000 )
			return;

		updateCondition(CONDITION_PUSH);

		if ( randomFloat(0,1.0) > 0.75f )
			m_nextVoicecmd = TF_VC_YES;

		break;
	default:
		break;
	}
}

void CBroadcastFlagCaptured :: execute ( CBot *pBot )
{
	if ( pBot->getTeam() == m_iTeam )
		((CBotTF2*)pBot)->flagReset();
	else
		((CBotTF2*)pBot)->teamFlagReset();
}

void CBroadcastRoundStart :: execute ( CBot *pBot )
{
	((CBotTF2*)pBot)->roundReset(m_bFullReset);
}

CBotFortress :: CBotFortress()
{ 
	CBot(); 

	// remember prev spy disguised in game while playing
	m_iPrevSpyDisguise = (TF_Class)0;

	m_fSentryPlaceTime = 0;
	m_iSentryKills = 0;
	m_fSnipeAttackTime = 0;
	m_pAmmo = NULL;
	m_pHealthkit = NULL;
	m_pFlag = NULL; 
	m_pHeal = NULL; 
	m_fCallMedic = 0; 
	m_fTauntTime = 0; 
	m_fLastKnownFlagTime = 0.0f; 
	m_bHasFlag = false; 
	m_pSentryGun = NULL; 
	m_pDispenser = NULL; 
	m_pTeleExit = NULL; 
	m_pTeleEntrance = NULL; 
	m_pNearestDisp = NULL;
	m_pNearestEnemySentry = NULL;
	m_pNearestEnemyTeleporter = NULL;
	m_pPrevSpy = NULL;
	m_fSeeSpyTime = 0.0f;
	m_bEntranceVectorValid = false;
	m_pLastCalledMedic = NULL;
	m_fLastCalledMedicTime = 0.0f;
	m_bIsBeingHealed = false;
	m_bCanBeUbered = false;
}

void CBotFortress :: checkDependantEntities ()
{
	CBot::checkDependantEntities();
}

void CBotFortress :: init (bool bVarInit)
{
	CBot::init(bVarInit);

	m_bCheckClass = false;
	m_bHasFlag = false;
	m_iClass = TF_CLASS_MAX; // important

}

void CBotFortress :: setup ()
{
	CBot::setup();
}

inline bool CBotFortress::someoneCalledMedic()
{
	return (getClass()==TF_CLASS_MEDIC) && 
			(m_pLastCalledMedic.get() != NULL) && 
			((m_fLastCalledMedicTime+30.0f)>engine->Time());
}

bool CBotFortress :: startGame()
{
	int team = m_pPlayerInfo->GetTeamIndex();
	
	m_iClass = (TF_Class)CClassInterface::getTF2Class(m_pEdict);

	if ( (team != TF2_TEAM_BLUE) && (team != TF2_TEAM_RED) )
	{
		selectTeam();
	}
	else if ( m_iDesiredClass == -1 ) // invalid class
	{
		chooseClass();
	}
	else if ( (m_iDesiredClass>0 && (m_iClass != m_iDesiredClass)) || (m_iClass == TF_CLASS_MAX) )
	{
		selectClass();
	}
	else
		return true;

	return false;
}

void CBotFortress ::pickedUpFlag()
{ 
	m_bHasFlag = true; 
	// clear tasks
	m_pSchedules->freeMemory();
}

void CBotFortress :: checkHealingValid ()
{
	if ( m_pHeal )
	{
		if ( !CBotGlobals::entityIsValid(m_pHeal) || !CBotGlobals::entityIsAlive(m_pHeal) )
		{
			m_pHeal = NULL;
			removeCondition(CONDITION_SEE_HEAL);
		}
		else if ( !isVisible(m_pHeal) )
		{
			m_pHeal = NULL;
			removeCondition(CONDITION_SEE_HEAL);
		}
		else if ( !wantToHeal(m_pHeal) )
		{
			m_pHeal = NULL;
			removeCondition(CONDITION_SEE_HEAL);
		}
	}
	else
		removeCondition(CONDITION_SEE_HEAL);
}

bool CBotFortress :: wantToHeal ( edict_t *pPlayer )
{
	IPlayerInfo *p = playerinfomanager->GetPlayerInfo(pPlayer);
	TF_Class iclass = (TF_Class)CClassInterface::getTF2Class(pPlayer);

	if ( ( iclass == TF_CLASS_SPY ) || (iclass == TF_CLASS_SNIPER ) || (iclass == TF_CLASS_MEDIC) || (iclass == TF_CLASS_ENGINEER) || (iclass == TF_CLASS_SCOUT) )
		return (ENTINDEX(pPlayer) <= gpGlobals->maxClients) && CBotGlobals::entityIsAlive(pPlayer) && (p->GetHealth() < p->GetMaxHealth());
	
	return (ENTINDEX(pPlayer) <= gpGlobals->maxClients) && CBotGlobals::entityIsAlive(pPlayer);
}


/////////////////////////////////////////////////////////////////////
//
// When a new Entity becomes visible or Invisible this is called
// 
// bVisible = true when pEntity is Visible
// bVisible = false when pEntity becomes inVisible
bool CBotFortress :: setVisible ( edict_t *pEntity, bool bVisible )
{
	bool bValid = CBot::setVisible(pEntity,bVisible);

	// check for people to heal
	if ( m_iClass == TF_CLASS_MEDIC )
	{
		if ( bValid && bVisible )
		{
			if ( ENTINDEX(pEntity) && (ENTINDEX(pEntity) <= gpGlobals->maxClients) ) // player
			{
				if ( CBotGlobals::getTeam(pEntity) == getTeam() )
				{
					Vector vPlayer = CBotGlobals::entityOrigin(pEntity);

					if ( distanceFrom(vPlayer) <= 384 )
					{
						if ( wantToHeal(pEntity) )
						{
							if ( m_pHeal )
							{
								if ( m_pHeal != pEntity )
								{
									IPlayerInfo *p1 = playerinfomanager->GetPlayerInfo(m_pHeal);
									IPlayerInfo *p2 = playerinfomanager->GetPlayerInfo(pEntity);

									if ( ((float)p2->GetHealth()/p2->GetMaxHealth()) < ((float)p1->GetHealth()/p1->GetMaxHealth()) )
									{									
										m_pHeal = pEntity;
										updateCondition(CONDITION_SEE_HEAL);
									}
								}					
							}
							else
							{
								m_pHeal = pEntity;
								updateCondition(CONDITION_SEE_HEAL);
							}
						}
					}
				}
			}
		}
		else if ( m_pHeal == pEntity )
		{
			m_pHeal = NULL;
			removeCondition(CONDITION_SEE_HEAL);
		}
	}
	else if ( m_iClass == TF_CLASS_SPY )
	{
		// Look for nearest sentry to sap!!!
		if ( bValid && bVisible )
		{
			if ( CTeamFortress2Mod::isSentry(pEntity,CTeamFortress2Mod::getEnemyTeam(getTeam())) )
			{
				if ( !CClassInterface::isObjectCarried(pEntity) )
				{
					if ( !m_pNearestEnemySentry || (pEntity != m_pNearestEnemySentry) && (distanceFrom(pEntity) < distanceFrom(m_pNearestEnemySentry)) )
					{
						m_pNearestEnemySentry = pEntity;
					}
				}
			}
			else if ( CTeamFortress2Mod::isTeleporter(pEntity,CTeamFortress2Mod::getEnemyTeam(getTeam())) )
			{
				if ( !m_pNearestEnemyTeleporter || (pEntity != m_pNearestEnemyTeleporter)&&(distanceFrom(pEntity)<distanceFrom(m_pNearestEnemyTeleporter)))
				{
					m_pNearestEnemyTeleporter = pEntity;
				}
			}
		}
		else if ( pEntity == m_pNearestEnemySentry )
		{
			m_pNearestEnemySentry = NULL;
		}
		else if ( pEntity == m_pNearestEnemyTeleporter )
		{
			m_pNearestEnemyTeleporter = NULL;
		}

	}

	// Check for nearest Dispenser for health/ammo & flag
	if ( bValid && bVisible && !(CClassInterface::getEffects(pEntity)&EF_NODRAW) ) // EF_NODRAW == invisible
	{
		if ( (m_pFlag!=pEntity) && CTeamFortress2Mod::isFlag(pEntity,getTeam()) )
			m_pFlag = pEntity;
		else if ( (m_pNearestAllySentry != pEntity) && CTeamFortress2Mod::isSentry(pEntity,getTeam()) )
		{
			if ( !m_pNearestAllySentry || (distanceFrom(pEntity) < distanceFrom(m_pNearestAllySentry))) 
				m_pNearestAllySentry = pEntity;
		}
		else if ( (m_pNearestDisp != pEntity) && CTeamFortress2Mod::isDispenser(pEntity,getTeam()) )
		{
			if ( !m_pNearestDisp || (distanceFrom(pEntity) < distanceFrom(m_pNearestDisp)) )
				m_pNearestDisp = pEntity;
		}
		else if ( (pEntity != m_pNearestTeleEntrance) && CTeamFortress2Mod::isTeleporterEntrance(pEntity,getTeam()) )
		{
			if ( !m_pNearestTeleEntrance || (distanceFrom(pEntity) < distanceFrom(m_pNearestTeleEntrance))) 
				m_pNearestTeleEntrance = pEntity;
		}
		else if ( (pEntity != m_pAmmo) && CTeamFortress2Mod::isAmmo(pEntity) )
		{
			static float fDistance;

			fDistance = distanceFrom(pEntity);

			if ( fDistance <= 200 )
			{
				if ( !m_pAmmo || (fDistance < distanceFrom(m_pAmmo))) 
					m_pAmmo = pEntity;
			}
			
		}
		else if ( (pEntity != m_pHealthkit) && CTeamFortress2Mod::isHealthKit(pEntity) )
		{
			static float fDistance;

			fDistance = distanceFrom(pEntity);

			if ( fDistance <= 200 )
			{
				if ( !m_pHealthkit || (fDistance < distanceFrom(m_pHealthkit)))
					m_pHealthkit = pEntity;
			}
		}
	}
	else 
	{
		if ( pEntity == m_pFlag )
			m_pFlag = NULL;
		else if ( pEntity == m_pNearestDisp )
			m_pNearestDisp = NULL;
		else if ( pEntity == m_pAmmo )
			m_pAmmo = NULL;
		else if ( pEntity == m_pHealthkit )
			m_pHealthkit = NULL;
		else if ( pEntity == m_pHeal )
			m_pHeal = NULL;
	}

	return bValid;
}

void CBotFortress :: medicCalled(edict_t *pPlayer )
{
	bool bGoto = true;

	if ( pPlayer == m_pEdict )
		return; // can't heal self!
	if ( m_iClass != TF_CLASS_MEDIC )
		return; // nothing to do
	if ( distanceFrom(pPlayer) > 1024 ) // a bit far away
		return; // ignore
	if ( (CBotGlobals::getTeam(pPlayer) == getTeam()) || (CClassInterface::getTF2Class(pPlayer) == TF_CLASS_SPY) && thinkSpyIsEnemy(pPlayer,CTeamFortress2Mod::getSpyDisguise(pPlayer)) )
	{

		m_pLastCalledMedic = pPlayer;
		m_fLastCalledMedicTime = engine->Time();

		if ( m_pHeal == pPlayer )
			return; // already healing


		if ( m_pHeal  )
		{
			if ( CClassInterface::getPlayerHealth(pPlayer) >= CClassInterface::getPlayerHealth(m_pHeal) )
				bGoto = false;
		}

		if ( bGoto )
		{
			m_pHeal = pPlayer;
		}

		m_pLastHeal = m_pHeal;

	}
}

void CBotFortress ::waitBackstab ()
{
	m_fBackstabTime = engine->Time() + randomFloat(5.0f,10.0f);
	m_pLastEnemy = NULL;
}

bool CBotFortress :: isAlive ()
{
	return !m_pPlayerInfo->IsDead()&&!m_pPlayerInfo->IsObserver();
}

// hurt enemy player
void CBotFortress :: seeFriendlyHurtEnemy ( edict_t *pTeammate, edict_t *pEnemy, CWeapon *pWeapon )
{
	if ( CClassInterface::getTF2Class(pEnemy) == TF_CLASS_SPY )
	{
		m_fSpyList[ENTINDEX(pEnemy)-1] = engine->Time();
	}
}

void CBotFortress :: shot ( edict_t *pEnemy )
{
	seeFriendlyHurtEnemy(m_pEdict,pEnemy,NULL);
}

void CBotFortress :: killed ( edict_t *pVictim, char *weapon )
{
	CBot::killed(pVictim,weapon);

	return;
}

void CBotFortress :: died ( edict_t *pKiller, const char *pszWeapon )
{
	CBot::died(pKiller,pszWeapon);

	if ( CClassInterface::getTF2Class(pKiller) == TF_CLASS_SPY )
		foundSpy(pKiller,(TF_Class)0);

	droppedFlag();

	if ( randomInt(0,1) )
		m_pButtons->attack();

	m_bCheckClass = true;
}

void CBotTF2 :: buildingDestroyed ( int iType, edict_t *pAttacker, edict_t *pEdict )
{
	eEngiBuild type = (eEngiBuild)iType;

	switch ( type )
	{
	case ENGI_DISP:
			m_pDispenser = NULL;
			m_bDispenserVectorValid = false;
			m_iDispenserArea = 0;

			break;
		case ENGI_SENTRY :
			m_pSentryGun = NULL;
			m_bSentryGunVectorValid = false;
			m_iSentryArea = 0;

			break;
		case ENGI_ENTRANCE :
			m_pTeleEntrance = NULL;
			m_bEntranceVectorValid = false;
			m_iTeleEntranceArea = 0;

			break;
		case ENGI_EXIT :
			m_pTeleExit = NULL;
			m_bTeleportExitVectorValid = false;
			m_iTeleExitArea = 0;
			break;
	}

	m_pSchedules->freeMemory();

	if ( pEdict && CBotGlobals::entityIsValid(pEdict) && pAttacker && CBotGlobals::entityIsValid(pAttacker) )
	{
		Vector vSentry = CBotGlobals::entityOrigin(pEdict);
		Vector vAttacker = CBotGlobals::entityOrigin(pAttacker);

		m_pNavigator->belief(vSentry,vAttacker,bot_beliefmulti.GetFloat(),(vAttacker-vSentry).Length(),BELIEF_DANGER);
	}
}

void CBotFortress ::wantToDisguise(bool bSet)
{
	if ( bSet )
		m_fSpyDisguiseTime = 0.0f;
	else
		m_fSpyDisguiseTime = engine->Time() + 2.0f;
}

void CBotFortress :: detectedAsSpy( edict_t *pDetector, bool bDisguiseComprimised )
{
	if ( bDisguiseComprimised )
	{
		float fTime = engine->Time() - m_fDisguiseTime;
		float fTotal = 0;

		if ( (m_fDisguiseTime < 1) || (fTime < 3.0f) )
			return;

		if ( m_fClassDisguiseTime[m_iDisguiseClass] == 0 )
			m_fClassDisguiseTime [m_iDisguiseClass] = fTime;
		else
			m_fClassDisguiseTime [m_iDisguiseClass] = (m_fClassDisguiseTime [m_iDisguiseClass] * 0.5f) + (fTime * 0.5f);

		for ( unsigned short int i = 0; i < 10; i ++ )
		{
			fTotal += m_fClassDisguiseTime[i];
		}
		
		for ( unsigned short int i = 0; i < 10; i ++ )
		{
			if ( m_fClassDisguiseTime[i] > 0 )
				m_fClassDisguiseFitness[i] = (m_fClassDisguiseTime[i] / fTotal);
		}

		m_fDisguiseTime = 0.0f;
	}
	else // go for cover
	{
		m_pAvoidEntity = pDetector;

		if ( !m_pSchedules->hasSchedule(SCHED_GOOD_HIDE_SPOT) )
		{
			m_pSchedules->freeMemory();
			m_pSchedules->addFront(new CGotoHideSpotSched(m_pAvoidEntity)); 
		}
	}
}

void CBotFortress :: spawnInit ()
{
	CBot::spawnInit();

	m_pLastSeeMedic.reset();

	memset(m_fSpyList,0,sizeof(float)*MAX_PLAYERS);

	m_fTaunting = 0.0f; // bots not moving FIX

	m_fMedicUpdatePosTime = 0.0f;

	m_pLastHeal = NULL;

	m_fDisguiseTime = 0.0f;

	m_pNearestEnemyTeleporter = NULL;
	m_pNearestTeleEntrance = NULL;
	m_fBackstabTime = 0.0f;
	m_fPickupTime = 0.0f;
	m_fDefendTime = 0.0f;
	m_fLookAfterSentryTime = 0.0f;

	m_fSnipeAttackTime = 0.0f;
	m_fSpyCloakTime = 0.0f; //engine->Time();// + randomFloat(5.0f,10.0f);
	m_fSpyUncloakTime = 0.0f;

	m_fLastSaySpy = 0.0f;
	m_fSpyDisguiseTime = 0.0f;
	m_pHeal = NULL;
	m_pNearestDisp = NULL;
	m_pNearestEnemySentry = NULL;
	m_pNearestAllySentry = NULL;
	m_bHasFlag = false; 
	//m_pPrevSpy = NULL;
	//m_fSeeSpyTime = 0.0f;

	m_bSentryGunVectorValid = false;
	m_bDispenserVectorValid = false;
	m_bTeleportExitVectorValid = false;

	
}

int CBotFortress :: engiBuildObject (int *iState, eEngiBuild iObject, float *fTime, int *iTries )
{
	switch ( *iState )
	{
	case 0:
		{
			// initialise
			if ( hasEngineerBuilt(iObject) )
			{
				engineerBuild(iObject,ENGI_DESTROY);
			}

			*iState = 1;
		}
		break;
	case 1:
		{
			CTraceFilterWorldAndPropsOnly filter;
			QAngle eyes = CBotGlobals::playerAngles(m_pEdict);
			QAngle turn;
			Vector forward;
			Vector building;
			// find best place to turn it to
			trace_t *tr = CBotGlobals::getTraceResult();
			int iNextState = 2;

			float bestfraction = 0.0f;			

			// unselect current weapon
			selectWeapon(0);
			engineerBuild(iObject,ENGI_BUILD);

			AngleVectors(eyes,&forward);
			iNextState = 8;
			building = getEyePosition() + (forward*100);
			//////////////////////////////////////////

			// forward
			CBotGlobals::traceLine(building,building + forward*4096.0,MASK_SOLID_BRUSHONLY,&filter);

			iNextState = 8;
			bestfraction = tr->fraction;

			////////////////////////////////////////
			turn = eyes;
			turn.y = turn.y - 90.0f;
			CBotGlobals::fixFloatAngle(&turn.y);

			AngleVectors(turn,&forward);

			// left
			CBotGlobals::traceLine(building,building + forward*4096.0,MASK_SOLID_BRUSHONLY,&filter);

			if ( tr->fraction > bestfraction )
			{
				iNextState = 6;
				bestfraction = tr->fraction;
			}
			////////////////////////////////////////
			turn = eyes;
			turn.y = turn.y + 180.0f;
			CBotGlobals::fixFloatAngle(&turn.y);

			AngleVectors(turn,&forward);

			// back
			CBotGlobals::traceLine(building,building + forward*4096.0,MASK_SOLID_BRUSHONLY,&filter);

			if ( tr->fraction > bestfraction )
			{
				iNextState = 4;
				bestfraction = tr->fraction;
			}
			///////////////////////////////////
			turn = eyes;
			turn.y = turn.y + 90.0f;
			CBotGlobals::fixFloatAngle(&turn.y);

			AngleVectors(turn,&forward);

			// right
			CBotGlobals::traceLine(building,building + forward*4096.0,MASK_SOLID_BRUSHONLY,&filter);

			if ( tr->fraction > bestfraction )
			{
				iNextState = 2;
				bestfraction = tr->fraction;
			}
			////////////////////////////////////
			*iState = iNextState;
		}
	case 2:
		{
			// let go
			m_pButtons->letGo(IN_ATTACK2);
			*iState = *iState + 1;
		}
		break;
	case 3:
		{
			tapButton(IN_ATTACK2);
			*iState = *iState + 1;
		}
		break;
	case 4:
		{
			// let go
			m_pButtons->letGo(IN_ATTACK2);
			*iState = *iState + 1;
		}
		break;
	case 5:
		{
			tapButton(IN_ATTACK2);
			*iState = *iState + 1;
		}
		break;
	case 6:

		{
			// let go 
			m_pButtons->letGo(IN_ATTACK2);
			*iState = *iState + 1;
		}
		break;
	case 7:
		{
			tapButton(IN_ATTACK2);
			*iState = *iState + 1;
		}
		break;
	case 8:
		{
			// let go (wait)
			m_pButtons->letGo(IN_ATTACK2);
			*iState = *iState + 1;
		}
		break;
	case 9:
		{
			tapButton(IN_ATTACK);

			*fTime = engine->Time() + randomFloat(0.5f,1.0f);

			*iState = *iState + 1;
		}
		break;
	case 10:
		{
			// Check if sentry built OK
			// Wait for Built object message

			m_pButtons->tap(IN_ATTACK);
			duck(true);// crouch too

			if ( *fTime < engine->Time() )
			{
				//hasbuiltobject
				if ( hasEngineerBuilt(iObject) )
				{
					*iState = *iState + 1;
					// OK, set up whacking time!
					*fTime = engine->Time() + randomFloat(5.0f,10.0f);

					if ( iObject == ENGI_SENTRY )
					{
						m_bSentryGunVectorValid = false;
					}
					else
					{
						if ( iObject == ENGI_DISP )
							m_bDispenserVectorValid = false;
						else if ( iObject == ENGI_EXIT )
							m_bTeleportExitVectorValid = false;

						return 1;
					}
				}
				else if ( *iTries > 3 )
				{
					if ( iObject == ENGI_SENTRY )
						m_bSentryGunVectorValid = false;
					else if ( iObject == ENGI_DISP )
						m_bDispenserVectorValid = false;
					else if ( iObject == ENGI_EXIT )
						m_bTeleportExitVectorValid = false;

					return 0;
				}
				else
				{
					*fTime = engine->Time() + randomFloat(0.5,1.0);
					*iTries = *iTries + 1;
					*iState = 1;
				}
			}
		}
		break;
	case 11:
		{
			// whack it for a while
			if ( *fTime < engine->Time() )
			{
				return 1;
			}
			else
			{
				tapButton(IN_ATTACK);
				duck(true);// crouch too
			}

			// someone blew my sentry before I built it!
			if ( !hasEngineerBuilt(iObject) )
				return 1;
		}
		break;
	}

	return 2;
}

void CBotFortress :: setClass ( TF_Class _class )
{
	m_iClass = _class;
}

bool CBotFortress :: thinkSpyIsEnemy ( edict_t *pEdict, TF_Class iDisguise )
{
	return ( (m_fSeeSpyTime > engine->Time()) &&  // if bot is in spy check mode
		(m_pPrevSpy == pEdict) &&				 // and its the last spy we saw
		// and its the same disguise or we last saw the spy just a couple of seconds ago
		((m_iPrevSpyDisguise == iDisguise)||((engine->Time()-m_fLastSeeSpyTime)<3.0f)) );
}

bool CBotTF2 ::thinkSpyIsEnemy(edict_t *pEdict, TF_Class iDisguise)
{
	return CBotFortress::thinkSpyIsEnemy(pEdict,iDisguise) || 
		(m_pCloakedSpy && (m_pCloakedSpy == pEdict) && !CTeamFortress2Mod::TF2_IsPlayerCloaked(m_pCloakedSpy)); // maybe i put him on fire
}

bool CBotFortress :: isEnemy ( edict_t *pEdict,bool bCheckWeapons )
{
	if ( pEdict == m_pEdict )
		return false;

	if ( !ENTINDEX(pEdict) || (ENTINDEX(pEdict) > CBotGlobals::maxClients()) )
		return false;

	if ( CBotGlobals::getTeam(pEdict) == getTeam() )
		return false;

	return true;	
}

bool CBotFortress :: needAmmo ()
{
	return false;
}

bool CBotFortress :: needHealth ()
{
	return getHealthPercent() < 0.7;
}

bool CBotTF2 :: needAmmo()
{
	if ( getClass() == TF_CLASS_ENGINEER )
	{		
		CBotWeapon *pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_WRENCH));

		if ( pWeapon )
		{
			int iMetal = pWeapon->getAmmo(this);

			if ( ( m_pSentryGun.get() == NULL ) || (CClassInterface::getTF2UpgradeLevel(m_pSentryGun) < 3) )
				return ( iMetal < 200 ); // need 200 to upgrade sentry
			else
				return iMetal < 125; // need 125 for other stuff (e.g. teleporters)
		}
	}
	else if ( getClass() == TF_CLASS_SOLDIER )
	{
		CBotWeapon *pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_ROCKETLAUNCHER));

		if ( pWeapon )
		{
			return ( pWeapon->getAmmo(this) < 1 );
		}
	}
	else if ( getClass() == TF_CLASS_DEMOMAN )
	{
		CBotWeapon *pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_GRENADELAUNCHER));

		if ( pWeapon )
		{
			return ( pWeapon->getAmmo(this) < 1 );
		}
	}
	else if ( getClass() == TF_CLASS_HWGUY )
	{
		CBotWeapon *pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_MINIGUN));

		if ( pWeapon )
		{
			return ( pWeapon->getAmmo(this) < 1 );
		}
	}
	else if ( getClass() == TF_CLASS_PYRO )
	{
		CBotWeapon *pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_FLAMETHROWER));

		if ( pWeapon )
		{
			return ( pWeapon->getAmmo(this) < 1 );
		}
	}

	return false;
}

void CBotFortress :: currentlyDead ()
{
	m_fUpdateClass = engine->Time() + 0.1f;
}

void CBotFortress :: modThink ()
{
	// get class
	m_iClass = (TF_Class)CClassInterface::getTF2Class(m_pEdict);
	m_iTeam = getTeam();
	//updateClass();

	if ( needHealth() )
		updateCondition(CONDITION_NEED_HEALTH);
	else
		removeCondition(CONDITION_NEED_HEALTH);

	if ( needAmmo() )
		updateCondition(CONDITION_NEED_AMMO);
	else
		removeCondition(CONDITION_NEED_AMMO);

	if ( !hasSomeConditions(CONDITION_PUSH) )
	{
		if ( CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict) || CTeamFortress2Mod::TF2_IsPlayerKrits(m_pEdict) )
			updateCondition(CONDITION_PUSH);
	}

	if ( m_fCallMedic < engine->Time() )
	{
		if ( ((float)m_pPlayerInfo->GetHealth() / m_pPlayerInfo->GetMaxHealth()) < 0.5 )
		{
			m_fCallMedic = engine->Time() + randomFloat(10.0f,30.0f);

			callMedic();
		}
	}

	if ( (m_fUseTeleporterTime < engine->Time() ) && !hasFlag() && m_pNearestTeleEntrance )
	{
		if ( isTeleporterUseful(m_pNearestTeleEntrance) )
		{
			if ( !m_pSchedules->isCurrentSchedule(SCHED_USE_TELE) )
			{
				m_pSchedules->freeMemory();
				//m_pSchedules->removeSchedule(SCHED_USE_TELE);
				m_pSchedules->addFront(new CBotUseTeleSched(m_pNearestTeleEntrance));

				m_fUseTeleporterTime = engine->Time() + randomFloat(25.0f,35.0f);
				return;
			}
		}
	}


	// Check redundant tasks
	if ( !hasSomeConditions(CONDITION_NEED_AMMO) && m_pSchedules->isCurrentSchedule(SCHED_TF2_GET_AMMO) )
	{
		m_pSchedules->removeSchedule(SCHED_TF2_GET_AMMO);
	}

	if ( !hasSomeConditions(CONDITION_NEED_HEALTH) && m_pSchedules->isCurrentSchedule(SCHED_TF2_GET_HEALTH) )
	{
		m_pSchedules->removeSchedule(SCHED_TF2_GET_HEALTH);
	}

	checkHealingValid();

	if ( m_bInitAlive )
	{
		Vector vOrigin = getOrigin();
		CWaypoint *pWpt = CWaypoints::getWaypoint(CWaypoints::nearestWaypointGoal(CWaypointTypes::W_FL_TELE_ENTRANCE,vOrigin,4096,m_iTeam));

		if ( pWpt )
		{
			// Get the nearest waypoint outside spawn (flagged as a teleporter entrance)
			// useful for Engineers and medics who want to camp for players
			m_vTeleportEntrance = pWpt->getOrigin();// + Vector(randomFloat(-pWpt->getRadius(),pWpt->getRadius()),randomFloat(-pWpt->getRadius(),pWpt->getRadius()),0);
			m_bEntranceVectorValid = true;
		}
	}

	if ( m_fLastSeeEnemy && ((m_fLastSeeEnemy + 5.0)<engine->Time()) )
	{
		m_fLastSeeEnemy = 0;
		m_pButtons->tap(IN_RELOAD);
	}
}


bool CBotFortress :: isTeleporterUseful ( edict_t *pTele )
{
	edict_t *pExit = CTeamFortress2Mod::getTeleporterExit(pTele);

	if ( pExit )
	{
		if ( !CClassInterface::isObjectBeingBuilt(pExit) && !CClassInterface::isObjectBeingBuilt(pTele) && !CClassInterface::isObjectCarried(pExit) && !CClassInterface::isObjectCarried(pTele) )
		{
			if ( ((m_vGoal - CBotGlobals::entityOrigin(pExit)).Length()+distanceFrom(pTele)) < distanceFrom(m_vGoal) )
				return true;
		}
	}

	return false;
}

void CBotFortress :: selectTeam ()
{
	char buffer[32];

	int team = randomInt(1,2);

	sprintf(buffer,"jointeam %d",team);

	helpers->ClientCommand(m_pEdict,buffer);
}

void CBotFortress :: selectClass ()
{
	char buffer[32];
	TF_Class _class;

	if ( m_iDesiredClass == 0 )
		_class = (TF_Class)randomInt(1,9);
	else
		_class = (TF_Class)m_iDesiredClass;

	m_iClass = _class;

	if ( _class == TF_CLASS_SCOUT )
	{
		sprintf(buffer,"joinclass scout");
	}
	else if ( _class == TF_CLASS_ENGINEER )
	{
		sprintf(buffer,"joinclass engineer");
	}
	else if ( _class == TF_CLASS_DEMOMAN )
	{
		sprintf(buffer,"joinclass demoman");
	}
	else if ( _class == TF_CLASS_SOLDIER )
	{
		sprintf(buffer,"joinclass soldier");
	}
	else if ( _class == TF_CLASS_HWGUY )
	{
		sprintf(buffer,"joinclass heavyweapons");
	}
	else if ( _class == TF_CLASS_MEDIC )
	{
		sprintf(buffer,"joinclass medic");
	}
	else if ( _class == TF_CLASS_SPY )
	{
		sprintf(buffer,"joinclass spy");
	}
	else if ( _class == TF_CLASS_PYRO )
	{
		sprintf(buffer,"joinclass pyro");
	}
	else
	{
		sprintf(buffer,"joinclass sniper");
	}
	helpers->ClientCommand(m_pEdict,buffer);

	m_fChangeClassTime = engine->Time() + randomFloat(bot_min_cc_time.GetFloat(),bot_max_cc_time.GetFloat());
}

bool CBotFortress :: waitForFlag ( Vector *vOrigin, float *fWait, bool bFindFlag )
{
	// job calls!
	if ( someoneCalledMedic() )
		return false;

	if ( seeFlag(false) != NULL )
	{
		edict_t *m_pFlag = seeFlag(false);

		if ( CBotGlobals::entityIsValid(m_pFlag) )
		{
			lookAtEdict(m_pFlag);
			setLookAtTask((LOOK_EDICT));
			*vOrigin = CBotGlobals::entityOrigin(m_pFlag);
			*fWait = engine->Time() + 5.0f;
		}
		else
			seeFlag(true);
	}
	else
		setLookAtTask(LOOK_AROUND);

	if ( distanceFrom(*vOrigin) > 48 )
		setMoveTo(*vOrigin);
	else
	{
		if ( !bFindFlag && ((getClass() == TF_CLASS_SPY) && isDisguised()) )
		{
			if ( !CTeamFortress2Mod::isFlagCarried(m_iTeam) )
				primaryAttack();
		}

		stopMoving();
	}

	return true;
	
		//taunt();
}

void CBotFortress :: foundSpy (edict_t *pEdict,TF_Class iDisguise) 
{
	m_pPrevSpy = pEdict;
	m_fSeeSpyTime = engine->Time() + randomFloat(9.0f,18.0f);
	m_vLastSeeSpy = CBotGlobals::entityOrigin(pEdict);
	m_fLastSeeSpyTime = engine->Time();
	if ( iDisguise && (m_iPrevSpyDisguise != iDisguise) )
		m_iPrevSpyDisguise = iDisguise;
	
	//m_fFirstSeeSpy = engine->Time(); // to do, add delayed action
};

// got shot by someone
bool CBotTF2 :: hurt ( edict_t *pAttacker, int iHealthNow, bool bDontHide )
{
	if ( !pAttacker )
		return false;

	if (( m_iClass != TF_CLASS_MEDIC ) || (!m_pHeal) )
	{
		if ( CBot::hurt(pAttacker,iHealthNow,true) )
		{
			if( m_bIsBeingHealed || m_bCanBeUbered )
			{
				// don't hide if I am being healed or can be ubered

				if ( m_bCanBeUbered && this->isTF2() && !hasFlag() ) // hack
					m_nextVoicecmd = TF_VC_ACTIVATEUBER;
			}
			else if ( !bDontHide )
			{
				if ( wantToNest() )
				{
					CBotSchedule *pSchedule = new CBotSchedule();

					pSchedule->setID(SCHED_GOOD_HIDE_SPOT);

					// run at flank while shooting	
					CFindPathTask *pHideGoalPoint = new CFindPathTask();
					Vector vOrigin = CBotGlobals::entityOrigin(pAttacker);
					
					// no interrupts, should be a quick waypoint path anyway
					pHideGoalPoint->setNoInterruptions();
					// get vector from good hide spot task
					pHideGoalPoint->getPassedVector();
					
					pSchedule->addTask(new CFindGoodHideSpot(vOrigin));
					pSchedule->addTask(pHideGoalPoint);
					pSchedule->addTask(new CBotNest());

					m_pSchedules->removeSchedule(SCHED_GOOD_HIDE_SPOT);
					m_pSchedules->addFront(pSchedule);
				}
				else
				{
					m_pSchedules->removeSchedule(SCHED_GOOD_HIDE_SPOT);
					m_pSchedules->addFront(new CGotoHideSpotSched(m_vHurtOrigin));
				}
			}

			return true;
		}
	}

	if ( pAttacker )
	{
		if ( !isCloaked() && !CTeamFortress2Mod::isSentry(pAttacker,CTeamFortress2Mod::getEnemyTeam(m_iTeam)) )
		{
			m_fFrenzyTime = engine->Time() + randomFloat(2.0f,6.0f);

			if ( isDisguised() )
				detectedAsSpy(pAttacker,true);
		}
	}

	return false;
}


/////////////////////////////////////////////////////////////////////////////
// TEAM FORTRESS 2

void CBotTF2 :: spawnInit()
{
	CBotFortress::spawnInit();

	m_fCarryTime = 0.0f;

	m_bIsCarryingTeleExit = false;
	 m_bIsCarryingSentry = false;;
	 m_bIsCarryingDisp = false;;
	 m_bIsCarryingTeleEnt = false;;
	 m_bIsCarryingObj = false;;

	m_nextVoicecmd = TF_VC_INVALID;
	m_fAttackPointTime = 0.0f;
	m_fNextRevMiniGunTime = 0.0f;
	m_fRevMiniGunTime = 0.0f;

	m_pCloakedSpy = NULL;

	m_fRemoveSapTime = 0.0f;

	// stickies destroyed now
	m_iTrapType = TF_TRAP_TYPE_NONE;

	//m_fBlockPushTime = 0.0f;

	m_iTeam = getTeam();
	// update current areas
	CPoints::getAreas(m_iTeam,&m_iCurrentDefendArea,&m_iCurrentAttackArea);

	m_fDoubleJumpTime = 0.0f;
	m_fFrenzyTime = 0.0f;
	m_fUseTeleporterTime = 0.0f;
	m_fSpySapTime = 0.0f;

	//m_pPushPayloadBomb = NULL;
	//m_pDefendPayloadBomb = NULL;

	m_bFixWeapons = true;
	m_iPrevWeaponSelectFailed = 0;

	m_fCheckNextCarrying = 0.0;

	
}

// return true if we don't want to hang around on the point
bool CBotTF2 ::checkAttackPoint()
{
	if ( CTeamFortress2Mod::isMapType(TF_MAP_CART) || CTeamFortress2Mod::isMapType(TF_MAP_CARTRACE) )
	{
		m_fAttackPointTime = engine->Time() + randomFloat(5.0f,15.0f);
		return true;
	}

	return false;
}

void CBotTF2 :: fixWeapons ()
{
	//if ( m_pWeapons && (m_iClass != TF_CLASS_MAX) )
	//{
	//	m_pWeapons->update();
		/*
		m_pWeapons->clearWeapons();
		
		switch ( m_iClass )
		{
		case TF_CLASS_SCOUT:

			m_pWeapons->addWeapon(TF2_WEAPON_BAT);
			m_pWeapons->addWeapon(TF2_WEAPON_PISTOL_SCOUT);
			m_pWeapons->addWeapon(TF2_WEAPON_SCATTERGUN);
		break;

		case TF_CLASS_ENGINEER:

			m_pWeapons->addWeapon(TF2_WEAPON_SHOTGUN_PRIMARY);
			m_pWeapons->addWeapon(TF2_WEAPON_PISTOL);
			m_pWeapons->addWeapon(TF2_WEAPON_WRENCH);
			//m_pWeapons->addWeapon(TF2_WEAPON_ENGIDESTROY);
			//m_pWeapons->addWeapon(TF2_WEAPON_ENGIBUILD);

		break;

		case TF_CLASS_HWGUY:

			m_pWeapons->addWeapon(TF2_WEAPON_SHOTGUN_HWG);
			m_pWeapons->addWeapon(TF2_WEAPON_MINIGUN);
			m_pWeapons->addWeapon(TF2_WEAPON_FISTS);

		break;

		case TF_CLASS_SPY:

			m_pWeapons->addWeapon(TF2_WEAPON_REVOLVER);
			m_pWeapons->addWeapon(TF2_WEAPON_KNIFE);
			m_pWeapons->addWeapon(TF2_WEAPON_BUILDER);

		break;

		case TF_CLASS_PYRO:

			m_pWeapons->addWeapon(TF2_WEAPON_FIREAXE);
			m_pWeapons->addWeapon(TF2_WEAPON_FLAMETHROWER);
			m_pWeapons->addWeapon(TF2_WEAPON_SHOTGUN_PYRO);

		break;

		case TF_CLASS_SOLDIER:

			m_pWeapons->addWeapon(TF2_WEAPON_ROCKETLAUNCHER);
			m_pWeapons->addWeapon(TF2_WEAPON_SHOTGUN_SOLDIER);
			m_pWeapons->addWeapon(TF2_WEAPON_SHOVEL);

		break;

		case TF_CLASS_SNIPER:

			m_pWeapons->addWeapon(TF2_WEAPON_SNIPERRIFLE);
			m_pWeapons->addWeapon(TF2_WEAPON_SMG);
			m_pWeapons->addWeapon(TF2_WEAPON_CLUB);

		break;

		case TF_CLASS_MEDIC:

			m_pWeapons->addWeapon(TF2_WEAPON_SYRINGEGUN);
			m_pWeapons->addWeapon(TF2_WEAPON_BONESAW);
			m_pWeapons->addWeapon(TF2_WEAPON_MEDIGUN);

		break;

		case TF_CLASS_DEMOMAN:

			m_pWeapons->addWeapon(TF2_WEAPON_PIPEBOMBS);
			m_pWeapons->addWeapon(TF2_WEAPON_GRENADELAUNCHER);
			m_pWeapons->addWeapon(TF2_WEAPON_BOTTLE);

		break;
		}*/

	//}
}

void CBotTF2 :: setClass ( TF_Class _class )
{
	m_iClass = _class;
}

// bOverride will be true in messaround mode
void CBotTF2 :: taunt ( bool bOverride )
{
	extern ConVar rcbot_taunt;
	// haven't taunted for a while, no emeny, not ubered, OK! Taunt!
	if ( bOverride || (!m_bHasFlag && rcbot_taunt.GetBool() && !CTeamFortress2Mod::TF2_IsPlayerOnFire(m_pEdict) && !m_pEnemy && (m_fTauntTime < engine->Time()) && (!CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict))) )
	{
		helpers->ClientCommand(m_pEdict,"taunt");
		m_fTauntTime = engine->Time() + randomFloat(40.0,100.0); // Don't taunt for another minute or two
		m_fTaunting = engine->Time() + 5.0;
	}
}

void CBotTF2::healedPlayer(edict_t *pPlayer, float fAmount)
{
	if ( m_iClass == TF_CLASS_ENGINEER ) // my dispenser was used
	{
		m_fDispenserHealAmount += fAmount;
	}
}
// useful for quick check of mod entities (especially ones which I need to find quickly
// e.g. sentry gun -- if I dont see it quickly it might kill me
edict_t *CBotFortress::getVisibleSpecial()
{
	static edict_t *pPlayer;
	static edict_t *pReturn;

	// this is a special visible which will return something important 
	// that should be visible quickly, e.g. an enemy sentry gun
	// or teleporter entrance for bots to make decisions quickly
	if ( (signed int)m_iSpecialVisibleId >= gpGlobals->maxClients )
		m_iSpecialVisibleId = 0;
	
	pPlayer = INDEXENT(m_iSpecialVisibleId+1);
	pReturn = NULL;

	if ( pPlayer && (CClassInterface::getTF2Class(pPlayer) == TF_CLASS_ENGINEER) )
	{
		edict_t *pSentry = CTeamFortress2Mod::getSentryGun (m_iSpecialVisibleId);

		// more interested in enemy sentries to sap and shoot!
		pReturn = pSentry;

		if ( CClassInterface::getTeam(pPlayer) == m_iTeam )
		{
			// more interested in teleporters on my team
			edict_t *pTele = CTeamFortress2Mod::getTeleEntrance (m_iSpecialVisibleId);

			pReturn = pTele;

			if ( getClass() == TF_CLASS_ENGINEER )
			{
				// be a bit more random with engi's as they both might be important
				// to repair other team members sentries!
				if ( !pTele || randomInt(0,1) )
					pReturn = pSentry;
			}

		}
	}

	m_iSpecialVisibleId++;

	return pReturn;
}
/*
lambda-
NEW COMMAND SYNTAX:
- "build 2 0" - Build sentry gun
- "build 0 0" - Build dispenser
- "build 1 0" - Build teleporter entrance
- "build 1 1" - Build teleporter exit
*/
void CBotTF2 :: engineerBuild ( eEngiBuild iBuilding, eEngiCmd iEngiCmd )
{
	//char buffer[16];
	//char cmd[256];

	if ( iEngiCmd == ENGI_BUILD )
	{
		//strcpy(buffer,"build");

		switch ( iBuilding )
		{
		case ENGI_DISP :
			//m_pDispenser = NULL;
			helpers->ClientCommand(m_pEdict,"build 0 0");
			break;
		case ENGI_SENTRY :
			//m_pSentryGun = NULL;
			helpers->ClientCommand(m_pEdict,"build 2 0");
			break;
		case ENGI_ENTRANCE :
			//m_pTeleEntrance = NULL;
			helpers->ClientCommand(m_pEdict,"build 1 0");
			break;
		case ENGI_EXIT :
			//m_pTeleExit = NULL;
			helpers->ClientCommand(m_pEdict,"build 1 1");
			break;
		default:
			return;
			break;
		}
	}
	else
	{
		//strcpy(buffer,"destroy");

		switch ( iBuilding )
		{
		case ENGI_DISP :
			m_pDispenser = NULL;
			m_bDispenserVectorValid = false;
			m_iDispenserArea = 0;
			helpers->ClientCommand(m_pEdict,"destroy 0 0");
			break;
		case ENGI_SENTRY :
			m_pSentryGun = NULL;
			m_bSentryGunVectorValid = false;
			m_iSentryArea = 0;
			helpers->ClientCommand(m_pEdict,"destroy 2 0");
			break;
		case ENGI_ENTRANCE :
			m_pTeleEntrance = NULL;
			m_bEntranceVectorValid = false;
			m_iTeleEntranceArea = 0;
			helpers->ClientCommand(m_pEdict,"destroy 1 0");
			break;
		case ENGI_EXIT :
			m_pTeleExit = NULL;
			m_bTeleportExitVectorValid = false;
			m_iTeleExitArea = 0;
			helpers->ClientCommand(m_pEdict,"destroy 1 1");
			break;
		default:
			return;
			break;
		}
	}

	//sprintf(cmd,"%s %d 0",buffer,iBuilding); // added extra value to end

	//helpers->ClientCommand(m_pEdict,cmd);
}

void CBotTF2 :: updateCarrying ()
{
	m_bIsCarryingTeleExit = (m_pTeleExit.get()!=NULL) && CClassInterface::isObjectCarried(m_pTeleExit);//(strcmp(&(m_pTeleExit.get()->GetIServerEntity()->GetModelName().ToCStr())[18],"teleporter_blueprint_exit.mdl")==0);
	m_bIsCarryingSentry = (m_pSentryGun.get()!=NULL) && CClassInterface::isObjectCarried(m_pSentryGun);// && (strcmp(&(m_pSentryGun.get()->GetIServerEntity()->GetModelName().ToCStr())[18],"sentry1_blueprint.mdl")==0);
	m_bIsCarryingDisp = (m_pDispenser.get()!=NULL) && CClassInterface::isObjectCarried(m_pDispenser);// && (strcmp(&(m_pDispenser.get()->GetIServerEntity()->GetModelName().ToCStr())[18],"dispenser_blueprint.mdl")==0);
	m_bIsCarryingTeleEnt = (m_pTeleEntrance.get()!=NULL) && CClassInterface::isObjectCarried(m_pTeleEntrance);// && (strcmp(&(m_pTeleEntrance.get()->GetIServerEntity()->GetModelName().ToCStr())[18],"teleporter_blueprint_enter.mdl")==0);
	m_bIsCarryingObj = m_bIsCarryingSentry || m_bIsCarryingTeleExit || m_bIsCarryingDisp || m_bIsCarryingTeleEnt;
		//m_fCheckNextCarrying = m_fCheckNextCarrying + 0.5f;
}

void CBotTF2 :: checkBuildingsValid (bool bForce) // force check carrying
{
	if ( m_pSentryGun )
	{
		if ( !CBotGlobals::entityIsValid(m_pSentryGun) || !CBotGlobals::entityIsAlive(m_pSentryGun) || !CTeamFortress2Mod::isSentry(m_pSentryGun,m_iTeam) )
		{
			m_pSentryGun = NULL;
			m_prevSentryHealth = 0;
			m_iSentryArea = 0;
		}
	}

	if ( m_pDispenser )
	{
		if ( !CBotGlobals::entityIsValid(m_pDispenser) || !CBotGlobals::entityIsAlive(m_pDispenser) || !CTeamFortress2Mod::isDispenser(m_pDispenser,m_iTeam) )
		{
			m_pDispenser = NULL;
			m_prevDispHealth = 0;
			m_iDispenserArea = 0;
		}
	}

	if ( m_pTeleEntrance )
	{
		if ( !CBotGlobals::entityIsValid(m_pTeleEntrance) || !CBotGlobals::entityIsAlive(m_pTeleEntrance) || !CTeamFortress2Mod::isTeleporterEntrance(m_pTeleEntrance,m_iTeam) )
		{
			m_pTeleEntrance = NULL;
			m_prevTeleEntHealth = 0;
			m_iTeleEntranceArea = 0;
		}
	}

	if ( m_pTeleExit )
	{
		if ( !CBotGlobals::entityIsValid(m_pTeleExit) || !CBotGlobals::entityIsAlive(m_pTeleExit) || !CTeamFortress2Mod::isTeleporterExit(m_pTeleExit,m_iTeam) )
		{
			m_pTeleExit = NULL;
			m_prevTeleExtHealth = 0;
			m_iTeleExitArea = 0;
		}
	}
}

// Find the EDICT_T of the building that the engineer just built...
edict_t *CBotTF2 :: findEngineerBuiltObject ( eEngiBuild iBuilding, int index )
{
	int team = getTeam();

	edict_t *pBest = INDEXENT(index);

	if ( pBest )
	{
		if ( iBuilding == ENGI_TELE )
		{
			if ( CTeamFortress2Mod::isTeleporterEntrance(pBest,team) )
				iBuilding = ENGI_ENTRANCE;
			else if ( CTeamFortress2Mod::isTeleporterExit(pBest,team) )
				iBuilding = ENGI_EXIT;
		}

		switch ( iBuilding )
		{
		case ENGI_DISP :
			m_pDispenser = pBest;
			break;
		case ENGI_ENTRANCE :
			m_pTeleEntrance = pBest;
			//m_vTeleportEntrance = CBotGlobals::entityOrigin(pBest);
			//m_bEntranceVectorValid = true;
			break;
		case ENGI_EXIT:
			m_pTeleExit = pBest;
			break;
		case ENGI_SENTRY :
			m_pSentryGun = pBest;
			break;
		default:
			return NULL;
		}
	}

	return pBest;
}

void CBotTF2 :: died ( edict_t *pKiller, const char *pszWeapon  )
{
	CBot::died(pKiller,pszWeapon);

	droppedFlag();

	if ( pKiller )
	{
		if ( CBotGlobals::entityIsValid(pKiller) )
		{
			m_pNavigator->belief(CBotGlobals::entityOrigin(pKiller),getEyePosition(),bot_beliefmulti.GetFloat(),distanceFrom(pKiller),BELIEF_DANGER);

			if ( !strcmp(pszWeapon,"obj_sentrygun") )
				m_pLastEnemySentry = CTeamFortress2Mod::getMySentryGun(pKiller);
		}
	}

	m_bCheckClass = true;
}

void CBotTF2 :: killed ( edict_t *pVictim, char *weapon )
{
	CBotFortress::killed(pVictim,weapon);

	if ( (m_pSentryGun.get() != NULL) && (m_iClass == TF_CLASS_ENGINEER) && weapon && *weapon && (strncmp(weapon,"obj_sentry",10) == 0) )
	{
		m_iSentryKills++;

		if ( pVictim && CBotGlobals::entityIsValid(pVictim) )
		{
			Vector vSentry = CBotGlobals::entityOrigin(m_pSentryGun);
			Vector vVictim = CBotGlobals::entityOrigin(pVictim);

			m_pNavigator->belief(vVictim,vSentry,bot_beliefmulti.GetFloat(),(vSentry-vVictim).Length(),BELIEF_SAFETY);
		}
	}
	else if ( pVictim && CBotGlobals::entityIsValid(pVictim) )
		m_pNavigator->belief(CBotGlobals::entityOrigin(pVictim),getEyePosition(),bot_beliefmulti.GetFloat(),distanceFrom(pVictim),BELIEF_SAFETY);

	taunt();
}

void CBotTF2 :: capturedFlag ()
{
	taunt();
}

void CBotTF2 :: spyDisguise ( int iTeam, int iClass )
{
	//char cmd[256];

	if ( iTeam == 3 )
		m_iImpulse = 230 + iClass;
	else if ( iTeam == 2 )
		m_iImpulse = 220 + iClass;

	m_fDisguiseTime = engine->Time();
	m_iDisguiseClass = iClass;
	m_fFrenzyTime = 0.0f; // reset frenzy time

	//moooo

	//sprintf(cmd,"disguise %d %d",iClass,iTeam);

	//helpers->ClientCommand(m_pEdict,cmd);
}
// Test
bool CBotTF2 :: isCloaked ()
{
	return CTeamFortress2Mod::TF2_IsPlayerCloaked(m_pEdict);
}
// Test
bool CBotTF2 :: isDisguised ()
{
	int _class,_team,_index,_health;

	if ( CClassInterface::getTF2SpyDisguised (m_pEdict,&_class,&_team,&_index,&_health) )

	{
		if ( _class > 0 )
			return true;
	}

	return false;
	
}

void CBotTF2 :: updateClass ()
{
	if ( m_fUpdateClass && (m_fUpdateClass < engine->Time()) )
	{
		/*const char *model = m_pPlayerInfo->GetModelName();

		if ( strcmp(model,"soldier") )
			m_iClass = TF_CLASS_SOLDIER;
		else if ( strcmp(model,"sniper") )
			m_iClass = TF_CLASS_SNIPER;
		else if ( strcmp(model,"heavyweapons") )
			m_iClass = TF_CLASS_HWGUY;
		else if ( strcmp(model,"medic") )
			m_iClass = TF_CLASS_MEDIC;
		else if ( strcmp(model,"pyro") )
			m_iClass = TF_CLASS_PYRO;
		else if ( strcmp(model,"spy") )
			m_iClass = TF_CLASS_SPY;
		else if ( strcmp(model,"scout") )
			m_iClass = TF_CLASS_SCOUT;
		else if ( strcmp(model,"engineer") )
			m_iClass = TF_CLASS_ENGINEER;
		else if ( strcmp(model,"demoman") )
			m_iClass = TF_CLASS_DEMOMAN;
		else
			m_iClass = TF_CLASS_CIVILIAN;
			*/

		m_fUpdateClass = 0;
	}
}

TF_Class CBotTF2 :: getClass ()
{
	return m_iClass;
}

void CBotTF2 :: setup ()
{
	CBotFortress::setup();
}

void CBotTF2 :: seeFriendlyDie ( edict_t *pDied, edict_t *pKiller, CWeapon *pWeapon )
{
	if ( pKiller && !m_pEnemy && !hasSomeConditions(CONDITION_SEE_CUR_ENEMY) )
	{
		//if ( pWeapon )
		//{
		//	DOD_Class pclass = (DOD_Class)CClassInterface::getPlayerClassDOD(pKiller);

			if ( pWeapon && (pWeapon->getID() == TF2_WEAPON_SENTRYGUN) )
			{
				addVoiceCommand(TF_VC_SENTRYAHEAD);
				updateCondition(CONDITION_COVERT);
				m_fCurrentDanger += 100.0f;
				m_pLastEnemySentry = CTeamFortress2Mod::getMySentryGun(pKiller);
				m_vLastDiedOrigin = CBotGlobals::entityOrigin(pDied);
				pKiller = m_pLastEnemySentry;
			}
			else 
			{
				addVoiceCommand(TF_VC_INCOMING);
				updateCondition(CONDITION_COVERT);
				m_fCurrentDanger += 50.0f;
			}

		//}

		// encourage bots to snoop out enemy or throw grenades
		m_fLastSeeEnemy = engine->Time();
		m_pLastEnemy = pKiller;
		m_fLastUpdateLastSeeEnemy = 0;
		m_vLastSeeEnemy = CBotGlobals::entityOrigin(m_pLastEnemy);
		m_vLastSeeEnemyBlastWaypoint = m_vLastSeeEnemy;

		CWaypoint *pWpt = CWaypoints::getWaypoint(CWaypointLocations::NearestBlastWaypoint(m_vLastSeeEnemy,getOrigin(),4096.0,-1,true,true,false,false,0,false));
			
		if ( pWpt )
			m_vLastSeeEnemyBlastWaypoint = pWpt->getOrigin();

		updateCondition(CONDITION_CHANGED);
	}
}


void CBotTF2 :: engiBuildSuccess ( eEngiBuild iBuilding, int index )
{
	edict_t *pEntity = findEngineerBuiltObject(iBuilding, index);

	if ( iBuilding == ENGI_SENTRY )
	{
		m_fSentryPlaceTime = engine->Time();
		m_iSentryKills = 0;
	}
	else if ( iBuilding == ENGI_DISP )
	{
		m_fDispenserPlaceTime = engine->Time();
		m_fDispenserHealAmount = 0.0f;
	}
	else if ( iBuilding == ENGI_TELE )
	{
		if ( CTeamFortress2Mod::isTeleporterEntrance(pEntity,m_iTeam) )
		{
			m_fTeleporterEntPlacedTime = engine->Time();

			//if ( m_pTeleExit.get() != NULL ) // already has exit built
			m_iTeleportedPlayers = 0;
		}
		else if ( CTeamFortress2Mod::isTeleporterExit(pEntity,m_iTeam) )
		{
			m_fTeleporterExtPlacedTime = engine->Time();

			if ( m_pTeleEntrance.get() == NULL ) // doesn't have entrance built
				m_iTeleportedPlayers = 0;
		}
	}
}

bool CBotTF2 :: hasEngineerBuilt ( eEngiBuild iBuilding )
{
	switch ( iBuilding )
	{
	case ENGI_SENTRY:
		return m_pSentryGun!=NULL; // TODO
		break;
	case ENGI_DISP:
		return m_pDispenser!=NULL; // TODO
		break;
	case ENGI_ENTRANCE:
		return m_pTeleEntrance!=NULL; // TODO
		break;
	case ENGI_EXIT:
		return m_pTeleExit!=NULL; // TODO
		break;
	}	

	return false;
}

void CBotFortress :: flagDropped ( Vector vOrigin )
{ 
	m_vLastKnownFlagPoint = vOrigin; 
	m_fLastKnownFlagTime = engine->Time() + 60.0f;

	if ( m_pSchedules->hasSchedule(SCHED_TF2_GET_FLAG) )
		m_pSchedules->removeSchedule(SCHED_TF2_GET_FLAG);
}

void CBotFortress :: teamFlagDropped ( Vector vOrigin )
{
	m_vLastKnownTeamFlagPoint = vOrigin; 
	m_fLastKnownTeamFlagTime = engine->Time() + 60.0f;
}

void CBotFortress :: callMedic ()
{
	helpers->ClientCommand (m_pEdict,"saveme");
}

bool CBotFortress :: canGotoWaypoint (Vector vPrevWaypoint, CWaypoint *pWaypoint)
{
	if ( CBot::canGotoWaypoint(vPrevWaypoint,pWaypoint) )
	{
		if ( pWaypoint->hasFlag(CWaypointTypes::W_FL_AREAONLY) )
		{
			if ( !CPoints::isValidArea(pWaypoint->getArea()) )
				return false;
		}
		
		if ( pWaypoint->hasFlag(CWaypointTypes::W_FL_ROCKET_JUMP) )
		{
			CBotWeapons *pWeapons = getWeapons();
			CBotWeapon *pWeapon;

			if (getHealthPercent() > 0.5)
			{
				// only soldiers or demomen can use these
				if ( getClass() == TF_CLASS_SOLDIER )
				{
					pWeapon = pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_ROCKETLAUNCHER));
					
					if ( pWeapon )
						return (pWeapon->getAmmo(this) > 0);
				}
				else if ( ( getClass() == TF_CLASS_DEMOMAN ) && rcbot_demo_jump.GetBool() )
				{
					pWeapon = pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_PIPEBOMBS));

					if ( pWeapon )
						return (pWeapon->getClip1(this) > 0);
				}
			}

			return false;
		}
		
		if ( pWaypoint->hasFlag(CWaypointTypes::W_FL_DOUBLEJUMP) )
		{
			return ( getClass() == TF_CLASS_SCOUT);
		}

		if ( pWaypoint->hasFlag(CWaypointTypes::W_FL_WAIT_OPEN) )
		{
			return ( CTeamFortress2Mod::isArenaPointOpen() );
		}

		if ( pWaypoint->hasFlag(CWaypointTypes::W_FL_NO_FLAG) )
		{
			return ( !hasFlag() );
		}

		if ( pWaypoint->hasFlag(CWaypointTypes::W_FL_FLAGONLY) )
		{
			return hasFlag();
		}

		return true;
	}
	else if ( pWaypoint->hasFlag(CWaypointTypes::W_FL_FALL) )
	{
		return ( getClass() == TF_CLASS_SCOUT );
	}

	return false;
}

void CBotTF2 :: callMedic ()
{
	addVoiceCommand(TF_VC_MEDIC);
}

void CBotFortress ::waitCloak()
{
	m_fSpyCloakTime = engine->Time() + randomFloat(2.0f,6.0f);
}

bool CBotFortress:: wantToCloak()
{	
	if ( ( m_fFrenzyTime < engine->Time() ) && (!m_pEnemy || !hasSomeConditions(CONDITION_SEE_CUR_ENEMY))  )
	{
		return ( (CClassInterface::getTF2SpyCloakMeter(m_pEdict) > 90.0f) && ( m_fCurrentDanger > TF2_SPY_CLOAK_BELIEF ));
	}

	return false;
}

bool CBotFortress:: wantToUnCloak ()
{
	if ( wantToShoot() && m_pEnemy && hasSomeConditions(CONDITION_SEE_CUR_ENEMY) )
	{
		// hopefully the enemy can't see me
		if ( CBotGlobals::isAlivePlayer(m_pEnemy) && ( fabs(CBotGlobals::yawAngleFromEdict(m_pEnemy,getOrigin())) > bot_spyknifefov.GetFloat() ) ) 
			return true;
		else if ( !m_pEnemy || !hasSomeConditions(CONDITION_SEE_CUR_ENEMY) )
			return (m_fCurrentDanger < 1.0f);
	}

	return false;
}


void CBotTF2 :: spyUnCloak ()
{
	if ( m_fSpyCloakTime < engine->Time() )
	{
		secondaryAttack();

		m_fSpyCloakTime = engine->Time() + randomFloat(2.0f,4.0f);
		//m_fSpyCloakTime = m_fSpyUncloakTime;
	}
}

void CBotTF2 ::spyCloak()
{
	if ( m_fSpyCloakTime < engine->Time() )
	{
		m_fSpyCloakTime = engine->Time() + randomFloat(2.0f,4.0f);
		//m_fSpyUncloakTime = m_fSpyCloakTime;

		secondaryAttack();
	}
}

void CBotFortress::chooseClass()
{
	float fClassFitness[10];
	float fTotalFitness = 0;
	float fRandom;

	int i = 0;
	int iTeam = getTeam();
	int iClass;
	edict_t *pPlayer;

	for ( i = 1; i < 10; i ++ )
		fClassFitness[i] = 1.0f;

	if ( (m_iClass >= 0) && (m_iClass < 10) )
		fClassFitness[m_iClass] = 0.1f;

	for ( i = 1; i <= gpGlobals->maxClients; i ++ )
	{
		pPlayer = INDEXENT(i);
		
		if ( CBotGlobals::entityIsValid(pPlayer) && (CTeamFortress2Mod::getTeam(pPlayer) == iTeam))
		{
			iClass = CClassInterface::getTF2Class(pPlayer);

			if ( (iClass >= 0) && (iClass < 10) )
				fClassFitness [iClass] *= 0.6f; 
		}
	}

	// attacking team?
	if ( CTeamFortress2Mod::isAttackDefendMap() )
	{
		if ( getTeam() == TF2_TEAM_BLUE )
		{
			fClassFitness[TF_CLASS_ENGINEER] *= 0.5;
			fClassFitness[TF_CLASS_SPY] *= 1.2;
			fClassFitness[TF_CLASS_SCOUT] *= 1.05;
		}
		else
		{
			fClassFitness[TF_CLASS_ENGINEER] *= 2.0;
			fClassFitness[TF_CLASS_SCOUT] *= 0.5;
			fClassFitness[TF_CLASS_HWGUY] *= 1.5;
			fClassFitness[TF_CLASS_MEDIC] *= 1.1;
		}
	}

	for ( int i = 1; i < 10; i ++ )
		fTotalFitness += fClassFitness[i];

	fRandom = randomFloat(0,fTotalFitness);

	fTotalFitness = 0;

	m_iDesiredClass = 0;

	for ( int i = 1; i < 10; i ++ )
	{
		fTotalFitness += fClassFitness[i];

		if ( fRandom <= fTotalFitness )
		{
			m_iDesiredClass = i;
			break;
		}
	}
}

void CBotTF2 :: modThink ()
{
	static bool bNeedHealth;
	static bool bNeedAmmo;
	static bool bIsCloaked;

	//if ( m_pWeapons ) // done in bot.cpp 
	//	m_pWeapons->update(false); // don't override ammo types from engine

	bNeedHealth = hasSomeConditions(CONDITION_NEED_HEALTH) && !m_bIsBeingHealed;
	bNeedAmmo = hasSomeConditions(CONDITION_NEED_AMMO);

	// mod specific think code here
	CBotFortress :: modThink();

	checkBeingHealed();

	if ( CTeamFortress2Mod::isMapType(TF_MAP_CARTRACE) )
	{
		if ( getTeam() == TF2_TEAM_BLUE )
		{
		    m_pDefendPayloadBomb = m_pRedPayloadBomb;
		    m_pPushPayloadBomb = m_pBluePayloadBomb;
		}
		else
		{
		    m_pDefendPayloadBomb = m_pBluePayloadBomb;
		    m_pPushPayloadBomb = m_pRedPayloadBomb;
		}
	}
	else if ( CTeamFortress2Mod::isMapType(TF_MAP_CART) )
	{
		if ( getTeam() == TF2_TEAM_BLUE )
		{
		    m_pPushPayloadBomb = m_pBluePayloadBomb;
			m_pDefendPayloadBomb = NULL;
		}
		else
		{
			m_pPushPayloadBomb = NULL;
			m_pDefendPayloadBomb = m_pBluePayloadBomb;
		}
	}

	// when respawned -- check if I should change class
	if ( m_bCheckClass && !m_pPlayerInfo->IsDead())
	{
		m_bCheckClass = false;

		if ( bot_change_class.GetBool() && (m_fChangeClassTime < engine->Time()) )
		{
				// get score for this class
				float scoreValue = CClassInterface::getTF2Score(m_pEdict);

				if ( m_iClass == TF_CLASS_ENGINEER )
				{
					if ( m_pSentryGun || (m_pTeleEntrance&&m_pTeleExit) )
					{
						//engineer bot is credit to team
						if ( CTeamFortress2Mod::isAttackDefendMap() )
						{
							if ( getTeam() == TF2_TEAM_BLUE )
							{
								scoreValue *= 1.25f;
							}
							else
								scoreValue *= 1.5f;
						}
						else
							scoreValue *= 1.5f; // less chance of changing class if bot has these up
					}
				}

				// if I think I could do better
				if ( randomFloat(0.0f,1.0f) > (scoreValue / CTeamFortress2Mod::getHighestScore()) )
				{
					chooseClass(); // edits m_iDesiredClass
					
					// change class
					selectClass();
				
				}
		}
	}

	m_fIdealMoveSpeed = CTeamFortress2Mod::TF2_GetPlayerSpeed(m_pEdict,m_iClass)*rcbot_speed_boost.GetFloat();

	// refind my weapons, if i couldn't select them
	//if ( m_bFixWeapons || (m_iPrevWeaponSelectFailed>2) )
	//{
	//	fixWeapons();
	//	m_bFixWeapons = false;
	//	m_iPrevWeaponSelectFailed = 0;
	//}//


	/* spy check code */
	if ( ((m_iClass != TF_CLASS_SPY)||(!isDisguised())) && ((m_pEnemy.get() == NULL) || !hasSomeConditions(CONDITION_SEE_CUR_ENEMY)) && (m_pPrevSpy.get() != NULL) && (m_fSeeSpyTime > engine->Time()) && 
		!m_bIsCarryingObj && CBotGlobals::isAlivePlayer(m_pPrevSpy) ) 
	{
		// check for spies within radius of bot / use aim skill as a skill factor
		float fPossibleDistance = (engine->Time()-m_fLastSeeSpyTime) * 
			(m_pProfile->m_fAimSkill * 310.0f) * (m_fCurrentDanger/MAX_BELIEF);

		// increase distance for pyro, he can use flamethrower !
		if ( m_iClass == TF_CLASS_PYRO ) 
			fPossibleDistance += 200.0f;

		if ( (m_vLastSeeSpy-getOrigin()).Length() < fPossibleDistance )
		{
			updateCondition(CONDITION_PARANOID);

			if ( m_pNavigator->hasNextPoint() && !m_pSchedules->isCurrentSchedule(SCHED_TF_SPYCHECK) )
			{
				CBotSchedule *newSchedule = new CBotSchedule(new CSpyCheckAir());

				newSchedule->setID(SCHED_TF_SPYCHECK);

				m_pSchedules->addFront(newSchedule);
			}
		}
		else
			removeCondition(CONDITION_PARANOID);
	}
	else
		removeCondition(CONDITION_PARANOID);

	if ( m_iClass == TF_CLASS_HWGUY )
	{
		static bool bRevMiniGun;
		
		bRevMiniGun = false;

		if ( wantToShoot() )
		{
			CBotWeapon *pWeapon = getCurrentWeapon();

			if ( pWeapon && (pWeapon->getID() == TF2_WEAPON_MINIGUN) )
			{
				if ( !CTeamFortress2Mod::TF2_IsPlayerOnFire(m_pEdict) &&
					!CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict) )
				{
					if ( m_fCurrentDanger >= TF2_HWGUY_REV_BELIEF )
					{
						if ( pWeapon->getAmmo(this) > 100 )
						{
							bRevMiniGun = true;
						}
					}
				}
			}
		}

		// Rev the minigun
		if ( bRevMiniGun )
		{
			// record time when bot started revving up
			if ( m_fRevMiniGunTime == 0 )
			{
				float fMinTime = (m_fCurrentDanger/200)*10;

				m_fRevMiniGunTime = engine->Time();
				m_fNextRevMiniGunTime = randomFloat(fMinTime,fMinTime+5.0f);
			}

			// rev for 10 seconds
			if ( (m_fRevMiniGunTime + m_fNextRevMiniGunTime) > engine->Time() )
			{
				secondaryAttack(true);
				m_fIdealMoveSpeed = 30.0f;

				if ( m_fCurrentDanger < 1 )
				{
					m_fRevMiniGunTime = 0.0f;
					m_fNextRevMiniGunTime = 0.0f;
				}
			}
			else if ( (m_fRevMiniGunTime + (2.0f*m_fNextRevMiniGunTime)) < engine->Time() )
			{
				m_fRevMiniGunTime = 0.0;
			}
		}
	}
	else if ( m_iClass == TF_CLASS_DEMOMAN )
	{
		if ( m_iTrapType != TF_TRAP_TYPE_NONE )
		{
			if ( m_pEnemy )
			{
				if ( (CBotGlobals::entityOrigin(m_pEnemy)-m_vStickyLocation).Length()<BLAST_RADIUS )
					detonateStickies();
			}
		}
	}
	else if ( m_iClass == TF_CLASS_SNIPER )
	{
		if ( CTeamFortress2Mod::TF2_IsPlayerZoomed(m_pEdict) )
			m_fFov = 20.0f; // Jagger
		else
			m_fFov = BOT_DEFAULT_FOV;
	}
	else if ( m_iClass == TF_CLASS_HWGUY )
	{
		if ( m_pButtons->holdingButton(IN_ATTACK) || m_pButtons->holdingButton(IN_ATTACK2) )
		{
			if ( m_pButtons->holdingButton(IN_JUMP) )
				m_pButtons->letGo(IN_JUMP);
		}
	}
	else if ( m_iClass == TF_CLASS_ENGINEER )
		checkBuildingsValid(false);
	else if ( m_iClass == TF_CLASS_SPY )
	{
		if ( !hasFlag() )
		{
			if ( m_fSpyDisguiseTime < engine->Time() )
			{
				// if previously detected or isn't disguised
				if ( (m_fDisguiseTime == 0.0f) || !isDisguised() )
				{
					int iteam = CTeamFortress2Mod::getEnemyTeam(getTeam());

					spyDisguise(iteam,getSpyDisguiseClass(iteam));
				}

				m_fSpyDisguiseTime = engine->Time() + 5.0f;
			}

			bIsCloaked = CTeamFortress2Mod::TF2_IsPlayerCloaked(m_pEdict);

			if ( bIsCloaked && wantToUnCloak() )
			{
				spyUnCloak();
			}
			else if ( !bIsCloaked && wantToCloak() )
			{
				spyCloak();
			}
			/*else if ( bIsCloaked && m_pEnemy && )
			{
				if ( CClassInterface::getTF2SpyCloakMeter(m_pEdict) < 
			}*/


			if ( m_pNearestEnemySentry && ( m_fSpySapTime < engine->Time() ) && !CTeamFortress2Mod::isSentrySapped(m_pNearestEnemySentry) && !m_pSchedules->hasSchedule(SCHED_SPY_SAP_BUILDING) )
			{
				m_fSpySapTime = engine->Time() + randomFloat(1.0f,4.0f);
				m_pSchedules->freeMemory();
				m_pSchedules->add(new CBotSpySapBuildingSched(m_pNearestEnemySentry,ENGI_SENTRY));
			}
			else if ( m_pNearestEnemyTeleporter && ( m_fSpySapTime < engine->Time() ) && !CTeamFortress2Mod::isTeleporterSapped(m_pNearestEnemyTeleporter) && !m_pSchedules->hasSchedule(SCHED_SPY_SAP_BUILDING) )
			{
				m_fSpySapTime = engine->Time() + randomFloat(1.0f,4.0f);
				m_pSchedules->freeMemory();
				m_pSchedules->add(new CBotSpySapBuildingSched(m_pNearestEnemyTeleporter,ENGI_TELE));
			}
		}
	}
	else if ( m_iClass == TF_CLASS_ENGINEER )
	{
		if ( !m_pSchedules->hasSchedule(SCHED_REMOVESAPPER) )
		{
			if ( (m_fRemoveSapTime<engine->Time()) && m_pNearestAllySentry && CBotGlobals::entityIsValid(m_pNearestAllySentry) && CTeamFortress2Mod::isSentrySapped(m_pNearestAllySentry) )
			{
					m_pSchedules->freeMemory();
					m_pSchedules->add(new CBotRemoveSapperSched(m_pNearestAllySentry,ENGI_SENTRY));
			}
			else if ( (m_fRemoveSapTime<engine->Time()) && m_pSentryGun && CBotGlobals::entityIsValid(m_pSentryGun) && CTeamFortress2Mod::isSentrySapped(m_pSentryGun) )
			{
				if ( distanceFrom(m_pSentryGun) < 1024.0f ) // only go back if I can remove the sapper
				{
					m_pSchedules->freeMemory();
					m_pSchedules->add(new CBotRemoveSapperSched(m_pSentryGun,ENGI_SENTRY));
				}
			}
		}
	}
	else if ( m_iClass == TF_CLASS_MEDIC )
	{
		if ( !hasFlag() && m_pHeal && CBotGlobals::entityIsAlive(m_pHeal) )
		{		
			if ( !m_pSchedules->hasSchedule(SCHED_HEAL) )
			{
				m_pSchedules->freeMemory();
				m_pSchedules->add(new CBotTF2HealSched(m_pHeal));
			}

			wantToShoot(false);
		}
	}

	// look for tasks / more important tasks here

	if ( !hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && !m_bLookedForEnemyLast && m_pLastEnemy && CBotGlobals::entityIsValid(m_pLastEnemy) && CBotGlobals::entityIsAlive(m_pLastEnemy) )
	{
		if ( wantToFollowEnemy() )
		{
			m_pSchedules->freeMemory();
			m_pSchedules->addFront(new CBotFollowLastEnemy(this, m_pLastEnemy,m_vLastSeeEnemy));
			m_bLookedForEnemyLast = true;
		}
	}

	if ( m_fTaunting > engine->Time() )
	{
		m_pButtons->letGoAllButtons(true);
		setMoveLookPriority(MOVELOOK_OVERRIDE);
		stopMoving();
		setMoveLookPriority(MOVELOOK_MODTHINK);
	}

	if ( m_fDoubleJumpTime && (m_fDoubleJumpTime < engine->Time()) )
	{
		tapButton(IN_JUMP);
		m_fDoubleJumpTime = 0;
	}

	if ( m_pSchedules->isCurrentSchedule(SCHED_GOTO_ORIGIN) && (m_fPickupTime < engine->Time()) && (bNeedHealth || bNeedAmmo) && (!m_pEnemy && !hasSomeConditions(CONDITION_SEE_CUR_ENEMY)) )
	{
		if ( (m_fPickupTime<engine->Time()) && m_pNearestDisp && !m_pSchedules->isCurrentSchedule(SCHED_USE_DISPENSER) )
		{
			if ( fabs(CBotGlobals::entityOrigin(m_pNearestDisp).z - getOrigin().z) < BOT_JUMP_HEIGHT )
			{
				m_pSchedules->removeSchedule(SCHED_USE_DISPENSER);
				m_pSchedules->addFront(new CBotUseDispSched(m_pNearestDisp));

				m_fPickupTime = engine->Time() + randomFloat(6.0f,20.0f);
				return;
			}
		}
		else if ( (m_fPickupTime<engine->Time()) && bNeedHealth && m_pHealthkit && !m_pSchedules->isCurrentSchedule(SCHED_TF2_GET_HEALTH) )
		{
			if ( fabs(CBotGlobals::entityOrigin(m_pHealthkit).z - getOrigin().z) < BOT_JUMP_HEIGHT )
			{
				m_pSchedules->removeSchedule(SCHED_TF2_GET_HEALTH);
				m_pSchedules->addFront(new CBotTF2GetHealthSched(CBotGlobals::entityOrigin(m_pHealthkit)));

				m_fPickupTime = engine->Time() + randomFloat(5.0f,10.0f);

				return;
			}
			
		}
		else if ( (m_fPickupTime<engine->Time()) && bNeedAmmo && m_pAmmo && !m_pSchedules->isCurrentSchedule(SCHED_PICKUP) )
		{
			if ( fabs(CBotGlobals::entityOrigin(m_pAmmo).z - getOrigin().z) < BOT_JUMP_HEIGHT )
			{
				m_pSchedules->removeSchedule(SCHED_TF2_GET_AMMO);
				m_pSchedules->addFront(new CBotTF2GetAmmoSched(CBotGlobals::entityOrigin(m_pAmmo)));

				m_fPickupTime = engine->Time() + randomFloat(5.0f,10.0f);

				return;
			}
		}
	}

	setMoveLookPriority(MOVELOOK_MODTHINK);
	
}

void CBotTF2::handleWeapons()
{
	if ( m_iClass == TF_CLASS_ENGINEER )
	{
		if ( m_bIsCarryingObj )
		{
			// don't shoot while carrying object unless after 5 seconds of carrying
			if ( (getHealthPercent() > 0.5f) || ((m_fCarryTime + 5.0f) > engine->Time()) )
				return;
		}
	}

	//
	// Handle attacking at this point
	//
	if ( m_pEnemy && !hasSomeConditions(CONDITION_ENEMY_DEAD) && 
		hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && wantToShoot() && 
		isVisible(m_pEnemy) && isEnemy(m_pEnemy) )
	{
		CBotWeapon *pWeapon;

		pWeapon = getBestWeapon(m_pEnemy,!hasFlag(),!hasFlag(),rcbot_melee_only.GetBool());

		setLookAtTask(LOOK_ENEMY);

		if ( m_bWantToChangeWeapon && (pWeapon != NULL) && (pWeapon != getCurrentWeapon()) && pWeapon->getWeaponIndex() )
		{
			//selectWeaponSlot(pWeapon->getWeaponInfo()->getSlot());
			selectWeapon(pWeapon->getWeaponIndex());
		}
		else
		{
			if ( !handleAttack ( pWeapon, m_pEnemy ) )
			{
				m_pEnemy = NULL;
				m_pOldEnemy = NULL;
				wantToShoot(false);
			}
		}
	}
}

void CBotTF2::enemyFound (edict_t *pEnemy)
{
	CBotFortress::enemyFound(pEnemy);
	m_fRevMiniGunTime = 0.0f;
}

bool CBotFortress :: canAvoid ( edict_t *pEntity )
{
	return CBot::canAvoid(pEntity);
}

bool CBotTF2::canAvoid(edict_t *pEntity)
{
	extern ConVar bot_avoid_radius;

	float distance;
	Vector vAvoidOrigin;
	int index;

	if ( !CBotGlobals::entityIsValid(pEntity) )
		return false;
	if ( m_pLookEdict == pEntity )
		return false;
	if ( m_pEdict == pEntity ) // can't avoid self!!!!
		return false;
	if ( m_pLastEnemy == pEntity )
		return false;
	if ( pEntity == m_pTeleEntrance )
		return false;
	if ( pEntity == m_pNearestTeleEntrance )
		return false;
	if ( pEntity == m_pNearestDisp )
		return false;
	if ( pEntity == m_pHealthkit )
		return false;
	if ( pEntity == m_pAmmo )
		return false;
	if (( pEntity == m_pSentryGun ) && ( CClassInterface::isObjectCarried(pEntity) ))
		return false;
	if (( pEntity == m_pDispenser ) && ( CClassInterface::isObjectCarried(pEntity) ))
		return false;
	if (( pEntity == m_pTeleExit ) && ( CClassInterface::isObjectCarried(pEntity) ))
		return false;

	edict_t *groundEntity = CClassInterface::getGroundEntity(m_pEdict);

	// must stand on worldspawn
	if ( groundEntity && (ENTINDEX(groundEntity)>0) && (pEntity == groundEntity) )
	{
		if ( pEntity == m_pSentryGun )
			return true;
		if ( pEntity == m_pDispenser )
			return true;
	}

	index = ENTINDEX(pEntity);

	if ( !index )
		return false;

	vAvoidOrigin = CBotGlobals::entityOrigin(pEntity);

	if ( vAvoidOrigin == m_vMoveTo )
		return false;

	distance = distanceFrom(vAvoidOrigin);

	if ( ( distance > 1 ) && ( distance < bot_avoid_radius.GetFloat() ) && (vAvoidOrigin.z >= getOrigin().z) && (fabs(getOrigin().z - vAvoidOrigin.z) < 64) )
	{
		if ( isEnemy(pEntity,false) )
			return true;
		else 
			return (pEntity==m_pSentryGun.get())||(pEntity==m_pDispenser.get());
	}

	return false;
}

bool CBotTF2:: wantToListenToPlayer ( edict_t *pPlayer )
{
	static edict_t *pWeapon;
	static const char *szWeaponClassname;
	
	pWeapon = CClassInterface::getCurrentWeapon(pPlayer);

	if ( !pWeapon )
		return true;

	szWeaponClassname = pWeapon->GetClassName();

	switch ( CClassInterface::getTF2Class(pPlayer) )
	{
		case TF_CLASS_MEDIC:
		{
			// don't listen to mediguns
			if ( !strcmp("medigun",&szWeaponClassname[10]) )
				return false;
		}
		break;
		case TF_CLASS_ENGINEER:
		{
			// don't listen to engis upgrading stuff
			if ( !strcmp("wrench",&szWeaponClassname[10]) )
				return false;
			else if ( !strcmp("builder",&szWeaponClassname[10]) )
				return false;
		}
		break;
		case TF_CLASS_SPY:
			{
				// don't listen to cloaked spies
				if ( CTeamFortress2Mod::TF2_IsPlayerCloaked(pPlayer) )
					return false;
			}
			break;

		default:
		break;
	}

	return true;
}

void CBotTF2::checkStuckonSpy(void)
{
	edict_t *pPlayer;
	edict_t *pStuck = NULL;

	int i = 0;
	int iTeam = getTeam();

	float fDistance;
	float fMaxDistance = 80;

	for ( i = 1; i <= gpGlobals->maxClients; i ++ )
	{
		pPlayer = INDEXENT(i);

		if ( pPlayer == m_pEdict )
			continue;

		if ( CBotGlobals::entityIsValid(pPlayer) && CBotGlobals::entityIsAlive(pPlayer) && (CTeamFortress2Mod::getTeam(pPlayer) != iTeam))
		{
			if ( (fDistance=distanceFrom(pPlayer)) < fMaxDistance ) // touching distance
			{
				if ( isVisible(pPlayer) )
				{
					fMaxDistance = fDistance;
					pStuck = pPlayer;
				}
			}
		}
	}

	if ( pStuck )
	{
		if ( CClassInterface::getTF2Class(pStuck) == TF_CLASS_SPY )
		{
			foundSpy(pStuck,CTeamFortress2Mod::getSpyDisguise(pStuck));
		}

		if ( (m_iClass == TF_CLASS_SPY) && isDisguised() )
		{
			 // Doh! found me!
			if ( randomFloat(0.0f,100.0f) < getHealthPercent() )
				m_fFrenzyTime = engine->Time() + randomFloat(0.0f,getHealthPercent());

			detectedAsSpy(pStuck,false);
			return;
		}
	}
}

bool CBotFortress :: isClassOnTeam ( int iClass, int iTeam )
{
	int i = 0;
	edict_t *pPlayer;

	for ( i = 1; i <= gpGlobals->maxClients; i ++ )
	{
		pPlayer = INDEXENT(i);

		if ( CBotGlobals::entityIsValid(pPlayer) && (CTeamFortress2Mod::getTeam(pPlayer) == iTeam))
		{
			if ( CClassInterface::getTF2Class(pPlayer) == iClass )
				return true;
		}
	}

	return false;
}

bool CBotFortress :: wantToFollowEnemy ()
{
	if ( hasSomeConditions(CONDITION_NEED_HEALTH) )
		return false;
	if ( hasSomeConditions(CONDITION_NEED_AMMO) )
		return false;
	if ( !CTeamFortress2Mod::hasRoundStarted() )
		return false;
    if ( !m_pLastEnemy )
        return false;
    if ( hasFlag() )
        return false;
    if ( m_iClass == TF_CLASS_SCOUT )
        return false;
	else if ( (m_iClass == TF_CLASS_MEDIC) && m_pHeal )
		return false;
    else if ( (m_iClass == TF_CLASS_SPY) && isDisguised() ) // sneak around the enemy
        return true;
	else if ( (m_iClass == TF_CLASS_SNIPER) && (distanceFrom(m_pLastEnemy)>CWaypointLocations::REACHABLE_RANGE) )
		return false; // don't bother!
	else if ( CBotGlobals::isPlayer(m_pLastEnemy) && (CClassInterface::getTF2Class(m_pLastEnemy) == TF_CLASS_SPY) && (thinkSpyIsEnemy(m_pLastEnemy,CTeamFortress2Mod::getSpyDisguise(m_pLastEnemy))) )
        return true; // always find spies!
	else if ( CTeamFortress2Mod::isFlagCarrier(m_pLastEnemy) )
		return true; // follow flag carriers to the death
	else if ( m_iClass == TF_CLASS_ENGINEER )
		return false; // have work to do
    
	return CBot::wantToFollowEnemy();
}

void CBotTF2 ::voiceCommand ( int cmd )
{
	char scmd[64];
	u_VOICECMD vcmd;

	vcmd.voicecmd = cmd;
	
	sprintf(scmd,"voicemenu %d %d",vcmd.b1.v1,vcmd.b1.v2);

	helpers->ClientCommand(m_pEdict,scmd);
}

bool CBotTF2 ::checkStuck(void)
{
	if ( !CTeamFortress2Mod::isAttackDefendMap() || (CTeamFortress2Mod::hasRoundStarted() || (getTeam()==TF2_TEAM_RED)) )
	{
		if ( CBot::checkStuck() )
		{
			checkStuckonSpy();

			return true;
		}
	}
	
	return false;
}

void CBotTF2 :: foundSpy (edict_t *pEdict,TF_Class iDisguise)
{
	CBotFortress::foundSpy(pEdict,iDisguise);

	if ( m_fLastSaySpy < engine->Time() )
	{
		addVoiceCommand(TF_VC_SPY);

		m_fLastSaySpy = engine->Time() + randomFloat(10.0f,40.0f);
	}
}

int CBotFortress :: getSpyDisguiseClass ( int iTeam )
{
	int i = 0;
	edict_t *pPlayer;
	dataUnconstArray<int> m_classes;
	int _class;
	float fTotal;
	float fRand;

	for ( i = 1; i <= gpGlobals->maxClients; i ++ )
	{
		pPlayer = INDEXENT(i);

		if ( CBotGlobals::entityIsValid(pPlayer) && (CTeamFortress2Mod::getTeam(pPlayer) == iTeam))
		{
			_class = CClassInterface::getTF2Class(pPlayer);

			if ( _class )
				m_classes.Add(_class);
		}
	}


	if ( m_classes.IsEmpty() )
		return randomInt(1,9);
	
	fTotal = 0;

	for ( int i = 0; i < m_classes.Size(); i ++ )
	{
		fTotal += m_fClassDisguiseFitness[m_classes.ReturnValueFromIndex(i)];
	}

	if ( fTotal > 0 )
	{

		fRand = randomFloat(0.0,fTotal);

		fTotal = 0;

		for ( int i = 0; i < m_classes.Size(); i ++ )
		{
			fTotal += m_fClassDisguiseFitness[m_classes.ReturnValueFromIndex(i)];

			if ( fRand <= fTotal )
				return m_classes.ReturnValueFromIndex(i);
		}

	}

	return m_classes.Random();
}

void CBotFortress :: enemyLost(edict_t *pEnemy)
{
	if ( CClassInterface::getTF2Class(pEnemy) == TF_CLASS_SPY )
	{
		if ( CBotGlobals::isAlivePlayer(pEnemy) )
		{
			updateCondition(CONDITION_CHANGED);
		}
	}
}

bool CBotTF2 :: setVisible ( edict_t *pEntity, bool bVisible )
{
	bool bValid = CBotFortress::setVisible(pEntity,bVisible);

	if ( bValid && bVisible )
	{
		if ( CTeamFortress2Mod::isPayloadBomb(pEntity,TF2_TEAM_RED) )
		{
			m_pRedPayloadBomb = pEntity;
		}
		else if ( CTeamFortress2Mod::isPayloadBomb(pEntity,TF2_TEAM_BLUE) )
		{
			m_pBluePayloadBomb = pEntity;
		}
	}

	if ( (ENTINDEX(pEntity)<=gpGlobals->maxClients) && (ENTINDEX(pEntity)>0) )
	{
		if ( bVisible )
		{
			TF_Class iPlayerclass = (TF_Class)CClassInterface::getTF2Class(pEntity);

			if ( iPlayerclass == TF_CLASS_SPY )
			{
				// check if disguise is not spy on my team
				int iClass, iTeam, iIndex, iHealth;
					
				CClassInterface::getTF2SpyDisguised(pEntity,&iClass,&iTeam,&iIndex,&iHealth);

				if ( CTeamFortress2Mod::TF2_IsPlayerCloaked(pEntity) )
				{
					if ( iClass != TF_CLASS_SPY ) // spies cloaking is normal / non spies cloaking is not!
					{
						if ( !m_pCloakedSpy || ((m_pCloakedSpy!=pEntity)&&(distanceFrom(pEntity) < distanceFrom(m_pCloakedSpy))) )
							m_pCloakedSpy = pEntity;
					}
				}
				else if ( ( iClass == TF_CLASS_MEDIC ) && ( iTeam == m_iTeam ) )
				{
					if ( !m_pLastSeeMedic.check(pEntity) && CBotGlobals::entityIsAlive(pEntity) )
					{
						// i think this spy can cure me!
						if ( (m_pLastSeeMedic.check(NULL) || (distanceFrom(pEntity) < distanceFrom(m_pLastSeeMedic.getLocation()))) && 
							 !thinkSpyIsEnemy(pEntity,(TF_Class)iClass) )
						{
							m_pLastSeeMedic = CBotLastSee(pEntity);
							/*m_pLastSeeMedic = pEntity;
							m_vLastSeeMedic = CBotGlobals::entityOrigin(pEntity);
							m_fLastSeeMedicTime = engine->Time();*/
						}
					}
					else
						m_pLastSeeMedic.update();

				}
			}
			else if ( iPlayerclass == TF_CLASS_MEDIC )
			{
				if ( m_pLastSeeMedic.check(pEntity) )
				{
					m_pLastSeeMedic.update();
					//m_fLastSeeMedicTime = engine->Time();
					//m_vLastSeeMedic = CBotGlobals::entityOrigin(pEntity);
				}
				else
				{
					if ( CBotGlobals::entityIsAlive(pEntity) && ((m_pLastSeeMedic.check(NULL)) || (distanceFrom(pEntity) < distanceFrom(m_pLastSeeMedic.getLocation()))) )
					{
						m_pLastSeeMedic = CBotLastSee(pEntity);
						//m_vLastSeeMedic = CBotGlobals::entityOrigin(pEntity);
						//m_fLastSeeMedicTime = engine->Time();
					}

				}
			}
		}
		else
		{
			if ( m_pCloakedSpy == pEntity )
				m_pCloakedSpy = NULL;
		}
	}

	return bValid;
	
}

void CBotTF2 :: checkBeingHealed ()
{
	static short i;
	static edict_t *p;
	static edict_t *pWeapon;
	static IPlayerInfo *pi;
	static const char *szWeaponName;
	

	if ( m_fCheckHealTime > engine->Time() )
		return;

	m_fCheckHealTime = engine->Time() + 1.0f;

	m_bIsBeingHealed = false;
	m_bCanBeUbered = false;

	for ( i = 1; i <= gpGlobals->maxClients; i ++ )
	{
		p = INDEXENT(i);

		if ( p == m_pEdict )
			continue;

		pi = playerinfomanager->GetPlayerInfo(p);

		if ( p && pi && (pi->GetTeamIndex()==getTeam()) && CBotGlobals::entityIsValid(p) && p->GetNetworkable()->GetClassName() )
		{
			szWeaponName = pi->GetWeaponName();

			if ( szWeaponName && *szWeaponName && strcmp(szWeaponName,"tf_weapon_medigun") == 0 )
			{
				pWeapon = CTeamFortress2Mod:: getMediGun(p);

				if ( !pWeapon )
					continue;

				if ( CClassInterface::getMedigunHealing(pWeapon) && (CClassInterface::isMedigunTargetting(pWeapon,m_pEdict)) )
				{
					if ( CClassInterface::getUberChargeLevel(pWeapon) > 99 )
						m_bCanBeUbered = true;

					m_bIsBeingHealed = true;
				}
			}
		}
	}

}

// Preconditions :  Current weapon is Medigun
//					pPlayer is not NULL
//
bool CBotTF2 :: healPlayer ( edict_t *pPlayer, edict_t *pPrevPlayer )
{
	static CBotWeapon *pWeap;
	static IPlayerInfo *p;
	static edict_t *pWeapon;
	static Vector vOrigin;
	static Vector vForward;
	static QAngle eyes;
	static float fSpeed;
	static CClient *pClient;
	
	if ( !m_pHeal )
		return false;
	
	if ( !wantToHeal(m_pHeal) )
		return false;

	vOrigin = CBotGlobals::entityOrigin(m_pHeal);
	pWeap = getCurrentWeapon();

	//if ( (distanceFrom(vOrigin) > 250) && !isVisible(m_pHeal) ) 
	//	return false;
	p = playerinfomanager->GetPlayerInfo(m_pHeal);

	if ( m_fMedicUpdatePosTime < engine->Time() )
	{
		float fRand;

		fRand = randomFloat(1.0f,2.0f);

		pClient = CClients::get(pPlayer);

		if ( pClient )
			fSpeed = pClient->getSpeed();

		m_fMedicUpdatePosTime = engine->Time() + (fRand * (1.0f-(fSpeed/320)));

		if ( p && (p->GetLastUserCommand().buttons & IN_ATTACK) )
		{
			// keep out of cross fire
			eyes = CBotGlobals::playerAngles(m_pHeal);
			AngleVectors(eyes,&vForward);
			vForward = vForward / vForward.Length();
			vOrigin = vOrigin - (vForward*150);
		}

		m_vMedicPosition = vOrigin;
	}

	if ( distanceFrom(m_vMedicPosition) < 100 )
		stopMoving();
	else
		setMoveTo(m_vMedicPosition);

	pWeapon = INDEXENT(pWeap->getWeaponIndex());

	if ( pWeapon == NULL )
		return false;

	if ( !CClassInterface::getMedigunHealing(pWeapon) )
	{
		if ( m_fHealClickTime < engine->Time() )
		{
			primaryAttack(true);
			m_fHealClickTime = engine->Time() + randomFloat(0.75f,1.25f);
		}
	}
	else
	{
		edict_t *pent;

		pPlayer = NULL;

		for ( unsigned short i = 1; i <= gpGlobals->maxClients; i++ )
		{
			pent = INDEXENT(i);

			if ( pent && CBotGlobals::entityIsValid(pent) )
			{
				if (CClassInterface::isMedigunTargetting(pWeapon,pent) )
				{
					pPlayer = pent;
					break;
				}
			}
		}

		if ( pPlayer )
		{
			m_pHeal = pPlayer;
			
			if ( pPrevPlayer != pPlayer )
			{
				if ( m_fHealClickTime < engine->Time() )
				{
					primaryAttack(true);
					m_fHealClickTime = engine->Time() + randomFloat(1.0f,2.0f);
				}
			}
		}
	}
	//else
	//	m_pHeal = CClassInterface::getMedigunTarget(INDEXENT(pWeap->getWeaponIndex()));

	lookAtEdict(m_pHeal);
	setLookAtTask(LOOK_EDICT);

	m_pLastHeal = m_pHeal;

	p = playerinfomanager->GetPlayerInfo(pPlayer);

	// Simple UBER check : healing player not ubered already
	if ( !CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pHeal) && !CTeamFortress2Mod::isFlagCarrier(m_pHeal) && 
		(m_pEnemy&&isVisible(m_pEnemy)) || (((((float)m_pPlayerInfo->GetHealth())/m_pPlayerInfo->GetMaxHealth())<0.33) || (getHealthPercent()<0.33) ))
	{
		if ( CTeamFortress2Mod::hasRoundStarted() )
		{
			// uber if ready / and round has started
			m_pButtons->tap(IN_ATTACK2);
		}
	}

	return true;
}
// The lower the better
float CBotTF2 :: getEnemyFactor ( edict_t *pEnemy )
{
	float fPreFactor = 0;

	// Player
	if ( ((ENTINDEX(pEnemy) > 0)&&(ENTINDEX(pEnemy)<=gpGlobals->maxClients)) )
	{		
		if ( CTeamFortress2Mod::isFlagCarrier(pEnemy) )
		{
			// this enemy is carrying the flag, attack!
			// shoot flag carrier even if 1000 units away from nearest enemy
			fPreFactor = -1000;
		}
		else if ( !CTeamFortress2Mod::isMapType(TF_MAP_CTF) && CTeamFortress2Mod::isCapping(pEnemy) )
		{
			// this enemy is capping the point, attack!
			fPreFactor = -1000;
		}
		else if ( CTeamFortress2Mod::TF2_IsPlayerInvuln(pEnemy) )
		{
			// dont shoot ubered player unlesss he's the only thing around for 1000 units
			fPreFactor = 2000;
		}
		else
		{
			int iclass = CClassInterface::getTF2Class(pEnemy);

			if ( iclass == TF_CLASS_MEDIC )
			{
				// shoot medic even if 250 units further from nearest enemy (approx. healing range)
				fPreFactor = -250;
			}
			else if ( iclass == TF_CLASS_SPY ) 
			{
				// shoot spy even if a little further from nearest enemy
				fPreFactor = -200;
			}
			else if ( iclass == TF_CLASS_SNIPER )
			{
				// I'm a spy, and I can see a sniper
				if ( m_iClass == TF_CLASS_SPY )
					fPreFactor = -600;
			}
			else if ( iclass == TF_CLASS_ENGINEER )
			{
				// I'm an engineer and I can see a spy
				if ( m_iClass == TF_CLASS_SPY )
					fPreFactor = -450;
			}
		}
	}
	else
	{
		if ( CTeamFortress2Mod::isSentry(pEnemy,CTeamFortress2Mod::getEnemyTeam(getTeam())) )
		{
			fPreFactor = -800.0f;
		}
		else if ( CTeamFortress2Mod::isBoss(pEnemy) )
		{
			extern ConVar bot_bossattackfactor;

			fPreFactor = -1500.0f * bot_bossattackfactor.GetFloat();
		}
	}

	fPreFactor += distanceFrom(pEnemy);

	return fPreFactor;
}


bool CBotFortress :: wantToNest ()
{
	return (!hasFlag() && ((getClass()!=TF_CLASS_ENGINEER)&&(m_pSentryGun.get()!=NULL)) && ((getClass() != TF_CLASS_MEDIC) || !m_pHeal) && (getHealthPercent() < 0.9) && (nearbyFriendlies(512.0f)<2));
}

void CBotTF2:: teleportedPlayer ( void )
{
	m_iTeleportedPlayers++;
}

void CBotTF2 :: getTasks ( unsigned int iIgnore )
{
	static TF_Class iClass;
	static int iMetal;
	static bool bNeedAmmo;
	static bool bNeedHealth;
	static CBotUtilities utils;
	static CBotWeapon *pWeapon;
	static CWaypoint *pWaypointResupply;
	static CWaypoint *pWaypointAmmo;
	static CWaypoint *pWaypointHealth;
	static CBotUtility *next;
	static float fResupplyDist;
	static float fHealthDist;
	static float fAmmoDist;
	static bool bHasFlag;
	static float fGetFlagUtility;
	static float fDefendFlagUtility;
	static int iTeam;
	static float fMetalPercent;
	static Vector vOrigin;
	static unsigned char *failedlist;

	extern ConVar rcbot_move_sentry_kpm;
	extern ConVar rcbot_move_sentry_time;
	extern ConVar rcbot_move_disp_time;
	extern ConVar rcbot_move_disp_healamount;
	extern ConVar rcbot_move_tele_time;
	extern ConVar rcbot_move_tele_tpm;
	extern ConVar rcbot_move_obj;

	static bool bMoveObjs;

	static int iSentryLevel;
	static int iDispenserLevel;
	static int iAllySentryLevel;
	static int iAllyDispLevel;

	static float fEntranceDist;
	static float fExitDist;
	static float fUseDispFactor;

	static float fAllyDispenserHealthPercent;
	static float fAllySentryHealthPercent;

	static float fSentryHealthPercent;
	static float fDispenserHealthPercent;
	static float fTeleporterEntranceHealthPercent;
	static float fTeleporterExitHealthPercent;

	static float fSentryPlaceTime;
	static float fDispenserPlaceTime;
	static float fTeleporterEntPlaceTime;
	static float fTeleporterExtPlaceTime;

	static float fSentryUtil;
	static int iMetalInDisp;


	static int numplayersonteam;
	static int numplayersonteam_alive;

	static bool bCheckCurrent;
	
	extern ConVar bot_messaround;
	extern ConVar bot_defrate;
	//static float fResupplyUtil = 0.5;
	//static float fHealthUtil = 0.5;
	//static float fAmmoUtil = 0.5;

	// if in setup time this will tell bot not to shoot yet
	wantToShoot(CTeamFortress2Mod::hasRoundStarted());
	m_bWantToListen = CTeamFortress2Mod::hasRoundStarted();

	if ( !hasSomeConditions(CONDITION_CHANGED) && !m_pSchedules->isEmpty() )
		return;
	
	removeCondition(CONDITION_CHANGED);

	bCheckCurrent = true; // important for checking the current schedule if not empty
	iMetal = 0;
	vOrigin = getOrigin();
	bNeedAmmo = false;
	bNeedHealth = false;
	fResupplyDist = 1;
	fHealthDist = 1;
	fAmmoDist = 1;
	bHasFlag = false;
	fGetFlagUtility = 0.5;
	fDefendFlagUtility = 0.5;
	iTeam = m_iTeam;
	bHasFlag = hasFlag();
	failedlist = NULL;

	numplayersonteam = CBotGlobals::numPlayersOnTeam(iTeam,false);
	numplayersonteam_alive = CBotGlobals::numPlayersOnTeam(iTeam,true);


	// UNUSED
	// Shadow/Time must be Floating point
	/*if(m_fBlockPushTime < engine->Time())
	{
		m_bBlockPushing = (randomFloat(0.0,100)>50); // 50 % block pushing
		m_fBlockPushTime = engine->Time() + randomFloat(10.0f,30.0f); // must be floating point
	}*/

	// No Enemy now
	if ( (m_iClass == TF_CLASS_SNIPER) && !hasSomeConditions(CONDITION_SEE_CUR_ENEMY) )
		// un zoom
	{
		if ( CTeamFortress2Mod::TF2_IsPlayerZoomed(m_pEdict) )
				secondaryAttack();
	}

	iClass = getClass();

	bNeedAmmo = hasSomeConditions(CONDITION_NEED_AMMO);

	// don't need health if being healed or ubered!
	bNeedHealth = hasSomeConditions(CONDITION_NEED_HEALTH) 
					&& !m_bIsBeingHealed && !CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict);

	if ( m_pHealthkit )
	{
		if ( !CBotGlobals::entityIsValid(m_pHealthkit) )
			m_pHealthkit = NULL;
	}

	if ( m_pAmmo )
	{
		if ( !CBotGlobals::entityIsValid(m_pAmmo) )
			m_pAmmo = NULL;
	}

	pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_WRENCH));
	iMetal = pWeapon->getAmmo(this);
	fMetalPercent = (float)iMetal/200;

	if ( bNeedAmmo || bNeedHealth )
	{
		dataUnconstArray<int> *failed;
		Vector vOrigin = getOrigin();

		m_pNavigator->getFailedGoals(&failed);

		failedlist = CWaypointLocations :: resetFailedWaypoints ( failed );

		fResupplyDist = 1;
		fHealthDist = 1;
		fAmmoDist = 1;

		pWaypointResupply = CWaypoints::getWaypoint(CWaypoints::getClosestFlagged(CWaypointTypes::W_FL_RESUPPLY,vOrigin,iTeam,&fResupplyDist,failedlist));
		
		if ( bNeedAmmo )
			pWaypointAmmo = CWaypoints::getWaypoint(CWaypoints::getClosestFlagged(CWaypointTypes::W_FL_AMMO,vOrigin,iTeam,&fAmmoDist,failedlist));
		if ( bNeedHealth )
			pWaypointHealth = CWaypoints::getWaypoint(CWaypoints::getClosestFlagged(CWaypointTypes::W_FL_HEALTH,vOrigin,iTeam,&fHealthDist,failedlist));
	}

	if ( iClass == TF_CLASS_ENGINEER )
	{
		checkBuildingsValid(true);
		updateCarrying();
	}

	ADD_UTILITY(BOT_UTIL_CAPTURE_FLAG,(CTeamFortress2Mod::isMapType(TF_MAP_CTF)||CTeamFortress2Mod::isMapType(TF_MAP_SD)) && bHasFlag,0.95f);

	if ( iClass == TF_CLASS_ENGINEER )
	{
		bMoveObjs = rcbot_move_obj.GetBool();

		iSentryLevel = 0;
		iDispenserLevel = 0;
		iAllySentryLevel = 0;
		iAllyDispLevel = 0;

		fEntranceDist = 99999.0f;
		fExitDist = 99999.0f;
		fUseDispFactor = 0.0f;

		fAllyDispenserHealthPercent = 1.0f;
		fAllySentryHealthPercent = 1.0f;

		fSentryHealthPercent = 1.0f;
		fDispenserHealthPercent = 1.0f;
		fTeleporterEntranceHealthPercent = 1.0f;
		fTeleporterExitHealthPercent = 1.0f;		

		fSentryPlaceTime = (engine->Time()-m_fSentryPlaceTime);
		fDispenserPlaceTime = (engine->Time()-m_fDispenserPlaceTime);
		fTeleporterEntPlaceTime = (engine->Time()-m_fTeleporterEntPlacedTime);
		fTeleporterExtPlaceTime = (engine->Time()-m_fTeleporterExtPlacedTime);


		if ( m_pTeleExit.get() )
		{
			fExitDist = distanceFrom(m_pTeleExit);

			fTeleporterExitHealthPercent = CClassInterface::getTeleporterHealth(m_pTeleExit)/180;

			ADD_UTILITY(BOT_UTIL_ENGI_MOVE_EXIT,(!m_bIsCarryingObj || m_bIsCarryingTeleExit) && bMoveObjs && m_pTeleEntrance && m_pTeleExit && 
				m_fTeleporterExtPlacedTime && (fTeleporterExtPlaceTime > rcbot_move_tele_time.GetFloat()) &&
				(((60.0f * m_iTeleportedPlayers)/fTeleporterExtPlaceTime)<rcbot_move_tele_tpm.GetFloat()),(fTeleporterExitHealthPercent*getHealthPercent()*fMetalPercent) + ((int)m_bIsCarryingTeleExit));

		}

		if ( m_pTeleEntrance.get() )
		{
			fEntranceDist = distanceFrom(m_pTeleEntrance);

			fTeleporterEntranceHealthPercent = CClassInterface::getTeleporterHealth(m_pTeleEntrance)/180;

			ADD_UTILITY(BOT_UTIL_ENGI_MOVE_ENTRANCE,(!m_bIsCarryingObj || m_bIsCarryingTeleEnt) && bMoveObjs && m_bEntranceVectorValid && m_pTeleEntrance && m_pTeleExit && 
				m_fTeleporterEntPlacedTime && (fTeleporterEntPlaceTime > rcbot_move_tele_time.GetFloat()) &&
				(((60.0f * m_iTeleportedPlayers)/fTeleporterEntPlaceTime)<rcbot_move_tele_tpm.GetFloat()),(fTeleporterEntranceHealthPercent*getHealthPercent()*fMetalPercent)+((int)m_bIsCarryingTeleEnt));

		}

		if ( m_pSentryGun.get() )
		{
			iSentryLevel = CClassInterface::getTF2UpgradeLevel(m_pSentryGun);//CTeamFortress2Mod::getSentryLevel(m_pSentryGun);
			fSentryHealthPercent = ((float)CClassInterface::getSentryHealth(m_pSentryGun))/CClassInterface::getTF2GetBuildingMaxHealth(m_pSentryGun);
			// move sentry
			ADD_UTILITY(BOT_UTIL_ENGI_MOVE_SENTRY,(!m_bIsCarryingObj || m_bIsCarryingSentry) && bMoveObjs && (m_fSentryPlaceTime>0.0f) && !bHasFlag && m_pSentryGun && (fSentryPlaceTime>rcbot_move_sentry_time.GetFloat())&&(((60.0f*m_iSentryKills)/fSentryPlaceTime)<rcbot_move_sentry_kpm.GetFloat()), (fMetalPercent*getHealthPercent()*fSentryHealthPercent)+((int)m_bIsCarryingSentry));

		}

		if ( m_pDispenser.get() )
		{
			iMetalInDisp = CClassInterface::getTF2DispMetal(m_pDispenser);
			iDispenserLevel = CClassInterface::getTF2UpgradeLevel(m_pDispenser); // CTeamFortress2Mod::getDispenserLevel(m_pDispenser);
			fDispenserHealthPercent = ((float)CClassInterface::getDispenserHealth(m_pDispenser)) / CClassInterface::getTF2GetBuildingMaxHealth(m_pDispenser);

			fUseDispFactor = (((float)iMetalInDisp)/400) * (1.0f-fMetalPercent) * ((float)iDispenserLevel/3) * (1000.0f/distanceFrom(m_pDispenser));

			// move disp
			ADD_UTILITY(BOT_UTIL_ENGI_MOVE_DISP,(!m_bIsCarryingObj || m_bIsCarryingDisp) && bMoveObjs && (m_fDispenserPlaceTime>0.0f) && !bHasFlag && m_pDispenser && (fDispenserPlaceTime>rcbot_move_disp_time.GetFloat())&&(((60.0f*m_fDispenserHealAmount)/fDispenserPlaceTime)<rcbot_move_disp_healamount.GetFloat()), ((((float)iMetalInDisp)/400)*fMetalPercent*getHealthPercent()*fDispenserHealthPercent) + ((int)m_bIsCarryingDisp));

		}

		if ( m_pNearestDisp && (m_pNearestDisp.get() != m_pDispenser.get()) )
		{
			iMetalInDisp = CClassInterface::getTF2DispMetal(m_pNearestDisp);
			iAllyDispLevel = CClassInterface::getTF2UpgradeLevel(m_pNearestDisp); // CTeamFortress2Mod::getDispenserLevel(m_pDispenser);
			fAllyDispenserHealthPercent = ((float)CClassInterface::getDispenserHealth(m_pNearestDisp)) / CClassInterface::getTF2GetBuildingMaxHealth(m_pNearestDisp);

			fUseDispFactor = (((float)iMetalInDisp)/400) * (1.0f-fMetalPercent) * ((float)iAllyDispLevel/3) * (1000.0f/distanceFrom(m_pNearestDisp));

			ADD_UTILITY(BOT_UTIL_GOTODISP,m_pNearestDisp && !CClassInterface::isObjectBeingBuilt(m_pNearestDisp) && (bNeedAmmo || bNeedHealth),fUseDispFactor);
			ADD_UTILITY(BOT_UTIL_REMOVE_TMDISP_SAPPER,!m_bIsCarryingObj && (m_fRemoveSapTime<engine->Time()) &&m_pNearestDisp && CTeamFortress2Mod::isDispenserSapped(m_pNearestDisp),1.1f);
			ADD_UTILITY(BOT_UTIL_UPGTMDISP,!m_bIsCarryingObj && (m_fRemoveSapTime<engine->Time()) && (m_pNearestDisp!=NULL)&&(m_pNearestDisp!=m_pDispenser) && (iMetal>=200) && ((iAllyDispLevel<3)||(fAllyDispenserHealthPercent<1.0f)),0.7+((1.0f-fAllyDispenserHealthPercent)*0.3));
		}

		if ( m_pNearestAllySentry && (m_pNearestAllySentry.get() != m_pSentryGun.get()) && !CClassInterface::getTF2BuildingIsMini(m_pNearestAllySentry) )
		{
			iAllySentryLevel = CClassInterface::getTF2UpgradeLevel(m_pNearestAllySentry);
			fAllySentryHealthPercent = CClassInterface::getSentryHealth(m_pNearestAllySentry);
			fAllySentryHealthPercent = fAllySentryHealthPercent / CClassInterface::getTF2GetBuildingMaxHealth(m_pNearestAllySentry);

			ADD_UTILITY(BOT_UTIL_REMOVE_TMSENTRY_SAPPER,!m_bIsCarryingObj && (m_fRemoveSapTime<engine->Time()) &&m_pNearestAllySentry && CTeamFortress2Mod::isSentrySapped(m_pNearestAllySentry),1.1f);
			ADD_UTILITY(BOT_UTIL_UPGTMSENTRY,!m_bIsCarryingObj && (m_fRemoveSapTime<engine->Time()) && !bHasFlag && m_pNearestAllySentry && (m_pNearestAllySentry!=m_pSentryGun) && (iMetal>=200) && ((iAllySentryLevel<3)||(fAllySentryHealthPercent<1.0f)),0.8+((1.0f-fAllySentryHealthPercent)*0.2));	
		}

		fSentryUtil = 0.8 + (((float)((int)bNeedAmmo))*0.1) + (((float)(int)bNeedHealth)*0.1);

		ADD_UTILITY(BOT_UTIL_PLACE_BUILDING, m_bIsCarryingObj, 1.0f); // something went wrong moving this- I still have it!!!

		// destroy and build anew
		ADD_UTILITY(BOT_UTIL_ENGI_DESTROY_SENTRY, !m_bIsCarryingObj && (iMetal>=130) && (m_pSentryGun.get()!=NULL) && !CPoints::isValidArea(m_iSentryArea),fSentryUtil);
		ADD_UTILITY(BOT_UTIL_ENGI_DESTROY_DISP, !m_bIsCarryingObj && (iMetal>=125) && (m_pDispenser.get()!=NULL) && !CPoints::isValidArea(m_iDispenserArea),randomFloat(0.7,0.9));
		ADD_UTILITY(BOT_UTIL_ENGI_DESTROY_ENTRANCE, !m_bIsCarryingObj && (iMetal>=125) && (m_pTeleEntrance.get()!=NULL) && !CPoints::isValidArea(m_iTeleEntranceArea),randomFloat(0.7,0.9));
		//ADD_UTILITY(BOT_UTIL_ENGI_DESTROY_EXIT, (iMetal>=125) && (m_pTeleExit.get()!=NULL) && !CPoints::isValidArea(m_iTeleExitArea),randomFloat(0.7,0.9));

		ADD_UTILITY(BOT_UTIL_BUILDSENTRY,!m_bIsCarryingObj && !bHasFlag && !m_pSentryGun && (iMetal>=130),0.9);
		ADD_UTILITY(BOT_UTIL_BUILDDISP,!m_bIsCarryingObj && !bHasFlag&& m_pSentryGun && !m_pDispenser && (iMetal>=100),fSentryUtil);
		
		if ( CTeamFortress2Mod::isAttackDefendMap() && (iTeam == TF2_TEAM_BLUE) )
		{
			ADD_UTILITY(BOT_UTIL_BUILDTELEXT,!m_bIsCarryingObj && !bHasFlag&&!m_pTeleExit&&(iMetal>=125),randomFloat(0.7,0.9));
			ADD_UTILITY(BOT_UTIL_BUILDTELENT,!m_bIsCarryingObj && !bHasFlag&&m_bEntranceVectorValid&&!m_pTeleEntrance&&(iMetal>=125),0.9f);
		}
		else
		{
			ADD_UTILITY(BOT_UTIL_BUILDTELENT,!m_bIsCarryingObj && !bHasFlag&&((m_pSentryGun.get()&&(iSentryLevel>1))||(m_pSentryGun.get()==NULL))&&m_bEntranceVectorValid&&!m_pTeleEntrance&&(iMetal>=125),0.9f);
			ADD_UTILITY(BOT_UTIL_BUILDTELEXT,!m_bIsCarryingObj && !bHasFlag&&m_pSentryGun&&(iSentryLevel>1)&&!m_pTeleExit&&(iMetal>=125),randomFloat(0.7,0.9));
		}

		ADD_UTILITY(BOT_UTIL_UPGSENTRY,!m_bIsCarryingObj && (m_fRemoveSapTime<engine->Time()) &&!bHasFlag &&( m_pSentryGun.get()!=NULL) && !CClassInterface::getTF2BuildingIsMini(m_pSentryGun) && (iMetal>=200) && ((iSentryLevel<3)||(fSentryHealthPercent<1.0f)),0.8+((1.0f-fSentryHealthPercent)*0.2));
		ADD_UTILITY(BOT_UTIL_GETAMMODISP,!m_bIsCarryingObj && m_pDispenser && isVisible(m_pDispenser) && (iMetal<200),fUseDispFactor);

		ADD_UTILITY(BOT_UTIL_UPGTELENT,!m_bIsCarryingObj && (m_fRemoveSapTime<engine->Time()) &&m_pTeleEntrance!=NULL && (iMetal>=200) &&  (fTeleporterEntranceHealthPercent<1.0f),((fEntranceDist<fExitDist)) * 0.51 + (0.5-(fTeleporterEntranceHealthPercent*0.5)));
		ADD_UTILITY(BOT_UTIL_UPGTELEXT,!m_bIsCarryingObj && (m_fRemoveSapTime<engine->Time()) &&m_pTeleExit!=NULL && (iMetal>=200) &&  (fTeleporterExitHealthPercent<1.0f),((fExitDist<fEntranceDist) * 0.51) + ((0.5-fTeleporterExitHealthPercent)*0.5));
		ADD_UTILITY(BOT_UTIL_UPGDISP,!m_bIsCarryingObj && (m_fRemoveSapTime<engine->Time()) &&m_pDispenser!=NULL && (iMetal>=200) && ((iDispenserLevel<3)||(fDispenserHealthPercent<1.0f)),0.7+((1.0f-fDispenserHealthPercent)*0.3));

		// remove sappers
		ADD_UTILITY(BOT_UTIL_REMOVE_SENTRY_SAPPER,!m_bIsCarryingObj && (m_fRemoveSapTime<engine->Time()) &&!bHasFlag&&(m_pSentryGun!=NULL) && CTeamFortress2Mod::isMySentrySapped(m_pEdict),1000.0f);
		ADD_UTILITY(BOT_UTIL_REMOVE_DISP_SAPPER,!m_bIsCarryingObj && (m_fRemoveSapTime<engine->Time()) &&!bHasFlag&&(m_pDispenser!=NULL) && CTeamFortress2Mod::isMyDispenserSapped(m_pEdict),1000.0f);

		ADD_UTILITY(BOT_UTIL_GOTORESUPPLY_FOR_AMMO, !m_bIsCarryingObj && !bHasFlag && pWaypointResupply && bNeedAmmo && !m_pAmmo,1000.0f/fResupplyDist);
		ADD_UTILITY(BOT_UTIL_FIND_NEAREST_AMMO,!m_bIsCarryingObj && !bHasFlag&&bNeedAmmo&&!m_pAmmo&&pWaypointAmmo,400.0f/fAmmoDist); // only if close

		ADD_UTILITY(BOT_UTIL_ENGI_LOOK_AFTER_SENTRY,!m_bIsCarryingObj && (m_pSentryGun!=NULL) && (iSentryLevel>2) && (m_fLookAfterSentryTime<engine->Time()),fGetFlagUtility+0.01f);

		// remove sappers

		ADD_UTILITY(BOT_UTIL_REMOVE_TMTELE_SAPPER,!m_bIsCarryingObj && (m_fRemoveSapTime<engine->Time()) &&m_pNearestTeleEntrance && CTeamFortress2Mod::isTeleporterSapped(m_pNearestTeleEntrance),1.1f);



// booooo
	}
	else
	{
		if ( !m_pNearestDisp )
		{
			m_pNearestDisp = CTeamFortress2Mod::nearestDispenser(getOrigin(),iTeam);
		}

		ADD_UTILITY(BOT_UTIL_GOTORESUPPLY_FOR_AMMO, !m_bIsCarryingObj && !bHasFlag && pWaypointResupply && bNeedAmmo && !m_pAmmo,1000.0f/fResupplyDist);
		ADD_UTILITY(BOT_UTIL_FIND_NEAREST_AMMO, !bHasFlag&&bNeedAmmo&&!m_pAmmo&&pWaypointAmmo,400.0f/fAmmoDist);

		if ( m_pNearestDisp )
			ADD_UTILITY(BOT_UTIL_GOTODISP,m_pNearestDisp && !CClassInterface::isObjectBeingBuilt(m_pNearestDisp) && (bNeedAmmo || bNeedHealth),1000.0f/distanceFrom(m_pNearestDisp));
	}

	fGetFlagUtility = 0.2+randomFloat(0.0f,0.2f);

	if ( m_iClass == TF_CLASS_SCOUT )
		fGetFlagUtility = 0.6f;
	else if ( m_iClass == TF_CLASS_SPY )
		fGetFlagUtility = 0.6f;
	else if ( m_iClass == TF_CLASS_MEDIC )
		fGetFlagUtility = 0.85f - (((float)numplayersonteam)/(gpGlobals->maxClients/2));
	else if ( m_iClass == TF_CLASS_ENGINEER )
		fGetFlagUtility = 0.1f; // not my priority

	fDefendFlagUtility = bot_defrate.GetFloat()/2;

	if ( (m_iClass == TF_CLASS_HWGUY) || (m_iClass == TF_CLASS_DEMOMAN) || (m_iClass == TF_CLASS_SOLDIER) || (m_iClass == TF_CLASS_PYRO) )
		fDefendFlagUtility = bot_defrate.GetFloat() - randomFloat(0.0f,fDefendFlagUtility);
	else if ( m_iClass == TF_CLASS_MEDIC )
		fDefendFlagUtility = fGetFlagUtility;

	if ( hasSomeConditions(CONDITION_PUSH) || CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict) )
	{
		fGetFlagUtility *= 2; 
		fDefendFlagUtility *= 2;
	}

	
	ADD_UTILITY(BOT_UTIL_GOTORESUPPLY_FOR_HEALTH, !m_bIsCarryingObj && !bHasFlag && pWaypointResupply && bNeedHealth && !m_pHealthkit,1000.0f/fResupplyDist);

	ADD_UTILITY(BOT_UTIL_GETAMMOKIT, bNeedAmmo && m_pAmmo,1.0);
	ADD_UTILITY(BOT_UTIL_GETHEALTHKIT, bNeedHealth && m_pHealthkit,1.0);

	ADD_UTILITY(BOT_UTIL_GETFLAG, (CTeamFortress2Mod::isMapType(TF_MAP_CTF)||(CTeamFortress2Mod::isMapType(TF_MAP_SD)&&CTeamFortress2Mod::canTeamPickupFlag_SD(iTeam,false))) && !bHasFlag,fGetFlagUtility);
	ADD_UTILITY(BOT_UTIL_GETFLAG_LASTKNOWN, (CTeamFortress2Mod::isMapType(TF_MAP_CTF)||(CTeamFortress2Mod::isMapType(TF_MAP_SD)&&CTeamFortress2Mod::canTeamPickupFlag_SD(iTeam,true))) && !bHasFlag && (m_fLastKnownFlagTime && (m_fLastKnownFlagTime > engine->Time())), fGetFlagUtility+0.1);

	ADD_UTILITY(BOT_UTIL_DEFEND_FLAG, CTeamFortress2Mod::isMapType(TF_MAP_MVM)||(CTeamFortress2Mod::isMapType(TF_MAP_CTF) && !bHasFlag), fDefendFlagUtility+0.1);
	ADD_UTILITY(BOT_UTIL_DEFEND_FLAG_LASTKNOWN, !bHasFlag &&
		(CTeamFortress2Mod::isMapType(TF_MAP_CTF) || CTeamFortress2Mod::isMapType(TF_MAP_MVM) ||
		(CTeamFortress2Mod::isMapType(TF_MAP_SD) && 
		(CTeamFortress2Mod::getFlagCarrierTeam()==CTeamFortress2Mod::getEnemyTeam(iTeam)))) &&
		(m_fLastKnownTeamFlagTime && (m_fLastKnownTeamFlagTime > engine->Time())), 
		fDefendFlagUtility+(randomFloat(0.0,0.2)-0.1));
	ADD_UTILITY(BOT_UTIL_SNIPE, (iClass==TF_CLASS_SNIPER) && !m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_SNIPERRIFLE))->outOfAmmo(this) && !hasSomeConditions(CONDITION_PARANOID) && !bHasFlag && (getHealthPercent()>0.2f), 0.95);	

	ADD_UTILITY(BOT_UTIL_ROAM,true,0.0001);
	ADD_UTILITY(BOT_UTIL_FIND_NEAREST_HEALTH,!bHasFlag&&bNeedHealth&&!m_pHealthkit&&pWaypointHealth,1000.0f/fHealthDist);
	
	ADD_UTILITY(BOT_UTIL_FIND_MEDIC_FOR_HEALTH,!bHasFlag && bNeedHealth && m_pLastSeeMedic.hasSeen(10.0f),1.0f);
	
	// only attack if attack area is > 0
	ADD_UTILITY(BOT_UTIL_ATTACK_POINT,(m_fAttackPointTime<engine->Time()) && 
		((m_iClass!=TF_CLASS_SPY)||!isDisguised()) && (m_iCurrentAttackArea>0) && 
			(CTeamFortress2Mod::isMapType(TF_MAP_SD)||CTeamFortress2Mod::isMapType(TF_MAP_CART)||
			CTeamFortress2Mod::isMapType(TF_MAP_CARTRACE)||
			(CTeamFortress2Mod::isMapType(TF_MAP_ARENA)&&CTeamFortress2Mod::isArenaPointOpen())||
			(CTeamFortress2Mod::isMapType(TF_MAP_KOTH)&&CTeamFortress2Mod::isArenaPointOpen())||
			CTeamFortress2Mod::isMapType(TF_MAP_CP)||CTeamFortress2Mod::isMapType(TF_MAP_TC)),fGetFlagUtility);

	// only defend if defend area is > 0
	// (!CTeamFortress2Mod::isAttackDefendMap()||(m_iTeam==TF2_TEAM_RED))
	ADD_UTILITY(BOT_UTIL_DEFEND_POINT, (m_iCurrentDefendArea>0) && 
		(CTeamFortress2Mod::isMapType(TF_MAP_MVM)||CTeamFortress2Mod::isMapType(TF_MAP_SD)||CTeamFortress2Mod::isMapType(TF_MAP_CART)||
		CTeamFortress2Mod::isMapType(TF_MAP_CARTRACE)||CTeamFortress2Mod::isMapType(TF_MAP_ARENA)||
		CTeamFortress2Mod::isMapType(TF_MAP_KOTH)||CTeamFortress2Mod::isMapType(TF_MAP_CP)||
		CTeamFortress2Mod::isMapType(TF_MAP_TC))&&m_iClass!=TF_CLASS_SCOUT,fDefendFlagUtility);

	ADD_UTILITY(BOT_UTIL_MEDIC_HEAL,(m_iClass == TF_CLASS_MEDIC) && !hasFlag() && m_pHeal && 
		CBotGlobals::entityIsAlive(m_pHeal) && wantToHeal(m_pHeal),0.98f);

	ADD_UTILITY(BOT_UTIL_MEDIC_HEAL_LAST,(m_iClass == TF_CLASS_MEDIC) && !hasFlag() && m_pLastHeal && 
		CBotGlobals::entityIsAlive(m_pLastHeal) && wantToHeal(m_pLastHeal),0.99f); 

	ADD_UTILITY(BOT_UTIL_HIDE_FROM_ENEMY,m_pEnemy && hasSomeConditions(CONDITION_SEE_CUR_ENEMY) &&
		!hasFlag() && (((m_iClass == TF_CLASS_MEDIC) && !m_pHeal) || 
		CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEnemy)),1.0f);

	ADD_UTILITY(BOT_UTIL_MEDIC_FINDPLAYER,(m_iClass == TF_CLASS_MEDIC) && 
		!m_pHeal && m_pLastCalledMedic && ((m_fLastCalledMedicTime+30.0f)>engine->Time()) && 
		( (numplayersonteam>1) && 
		  (numplayersonteam>CTeamFortress2Mod::numClassOnTeam(iTeam,getClass())) ),0.95f);

	ADD_UTILITY(BOT_UTIL_MEDIC_FINDPLAYER_AT_SPAWN,(m_iClass == TF_CLASS_MEDIC) && 
		!m_pHeal && !m_pLastCalledMedic && m_bEntranceVectorValid && (numplayersonteam>1) && 
		((!CTeamFortress2Mod::isAttackDefendMap() && !CTeamFortress2Mod::hasRoundStarted()) || (numplayersonteam_alive < numplayersonteam)),0.94f);

	if ( m_iClass==TF_CLASS_DEMOMAN )
	{
		if ( !hasEnemy() && ( m_iTrapType != TF_TRAP_TYPE_ENEMY ) )
		{
			ADD_UTILITY(BOT_UTIL_PIPE_LAST_ENEMY,(m_pLastEnemy!=NULL) && (distanceFrom(m_pLastEnemy) > (BLAST_RADIUS)),0.8f);
			ADD_UTILITY(BOT_UTIL_PIPE_NEAREST_SENTRY,(m_pNearestEnemySentry!=NULL) && (distanceFrom(m_pNearestEnemySentry) > (BLAST_RADIUS)),0.81f);
			ADD_UTILITY(BOT_UTIL_PIPE_LAST_ENEMY_SENTRY,(m_pLastEnemySentry!=NULL) && (distanceFrom(m_pLastEnemySentry) > (BLAST_RADIUS)),0.82f);
		}
	}

	if ( m_iClass==TF_CLASS_SPY )
	{
		ADD_UTILITY(BOT_UTIL_BACKSTAB,!hasFlag() && (!m_pNearestEnemySentry || (CTeamFortress2Mod::isSentrySapped(m_pNearestEnemySentry))) && (m_fBackstabTime<engine->Time()) && (m_iClass==TF_CLASS_SPY) && 
		((m_pEnemy&& CBotGlobals::isAlivePlayer(m_pEnemy))|| 
		(m_pLastEnemy&& CBotGlobals::isAlivePlayer(m_pLastEnemy))),
		fGetFlagUtility+(getHealthPercent()/10));

		ADD_UTILITY(BOT_UTIL_SAP_ENEMY_SENTRY,
										m_pEnemy && CTeamFortress2Mod::isSentry(m_pEnemy,CTeamFortress2Mod::getEnemyTeam(iTeam)) && !CTeamFortress2Mod::isSentrySapped(m_pEnemy),
										fGetFlagUtility+(getHealthPercent()/5));

		ADD_UTILITY(BOT_UTIL_SAP_NEAREST_SENTRY,m_pNearestEnemySentry && 
			!CTeamFortress2Mod::isSentrySapped(m_pNearestEnemySentry),
			fGetFlagUtility+(getHealthPercent()/5));

		ADD_UTILITY(BOT_UTIL_SAP_LASTENEMY_SENTRY,
			m_pLastEnemy && CTeamFortress2Mod::isSentry(m_pLastEnemy,CTeamFortress2Mod::getEnemyTeam(iTeam)) && !CTeamFortress2Mod::isSentrySapped(m_pLastEnemy),fGetFlagUtility+(getHealthPercent()/5));

		ADD_UTILITY(BOT_UTIL_SAP_LASTENEMY_SENTRY,
			m_pLastEnemySentry.get()!=NULL,fGetFlagUtility+(getHealthPercent()/5));
		////////////////
		// sap tele
		ADD_UTILITY(BOT_UTIL_SAP_ENEMY_TELE,
										m_pEnemy && CTeamFortress2Mod::isTeleporter(m_pEnemy,CTeamFortress2Mod::getEnemyTeam(iTeam)) && !CTeamFortress2Mod::isTeleporterSapped(m_pEnemy),
										fGetFlagUtility+(getHealthPercent()/6));

		ADD_UTILITY(BOT_UTIL_SAP_NEAREST_TELE,m_pNearestEnemyTeleporter && 
			!CTeamFortress2Mod::isTeleporterSapped(m_pNearestEnemyTeleporter),
			fGetFlagUtility+(getHealthPercent()/6));

		ADD_UTILITY(BOT_UTIL_SAP_LASTENEMY_TELE,
			m_pLastEnemy && CTeamFortress2Mod::isTeleporter(m_pLastEnemy,CTeamFortress2Mod::getEnemyTeam(iTeam)) && !CTeamFortress2Mod::isTeleporterSapped(m_pLastEnemy),fGetFlagUtility+(getHealthPercent()/6));
	}

	//fGetFlagUtility = 0.2+randomFloat(0.0f,0.2f);

	if ( CTeamFortress2Mod::isMapType(TF_MAP_CARTRACE) )
	{
		if ( iTeam == TF2_TEAM_BLUE )
		{
			ADD_UTILITY(BOT_UTIL_DEFEND_PAYLOAD_BOMB,((m_iClass!=TF_CLASS_SPY)||!isDisguised()) && (m_pDefendPayloadBomb!=NULL),fDefendFlagUtility+randomFloat(-0.1,0.2));
			ADD_UTILITY(BOT_UTIL_PUSH_PAYLOAD_BOMB,((m_iClass!=TF_CLASS_SPY)||!isDisguised()) && (m_pPushPayloadBomb!=NULL),fGetFlagUtility+randomFloat(-0.1,0.2));
		}
		else
		{
			ADD_UTILITY(BOT_UTIL_DEFEND_PAYLOAD_BOMB,((m_iClass!=TF_CLASS_SPY)||!isDisguised()) && (m_pDefendPayloadBomb!=NULL),fDefendFlagUtility+randomFloat(-0.1,0.2));
			ADD_UTILITY(BOT_UTIL_PUSH_PAYLOAD_BOMB,((m_iClass!=TF_CLASS_SPY)||!isDisguised()) && (m_pPushPayloadBomb!=NULL),fGetFlagUtility+randomFloat(-0.1,0.2));
		}
	}
	else if ( CTeamFortress2Mod::isMapType(TF_MAP_CART) )
	{
		if ( iTeam == TF2_TEAM_BLUE )
		{
			ADD_UTILITY(BOT_UTIL_PUSH_PAYLOAD_BOMB,((m_iClass!=TF_CLASS_SPY)||!isDisguised()) && (m_pPushPayloadBomb!=NULL),
				fGetFlagUtility+ (hasSomeConditions(CONDITION_PUSH)?0.25f:randomFloat(-0.1f,0.2f)));
			// Goto Payload bomb
		}
		else
		{
			// Defend Payload bomb
			ADD_UTILITY(BOT_UTIL_DEFEND_PAYLOAD_BOMB,
				((m_iClass!=TF_CLASS_SPY)||!isDisguised()) && (m_pDefendPayloadBomb!=NULL),fDefendFlagUtility+ 
				(hasSomeConditions(CONDITION_PUSH)?0.25f:randomFloat(-0.1f,0.2f)));
		}
	}

	if( ( m_iClass==TF_CLASS_DEMOMAN ) && (m_iTrapType==TF_TRAP_TYPE_NONE) )
	{
		ADD_UTILITY(BOT_UTIL_DEMO_STICKYTRAP_LASTENEMY,m_pLastEnemy && 
			(m_iTrapType==TF_TRAP_TYPE_NONE) && canDeployStickies(),
			randomFloat(min(fDefendFlagUtility,fGetFlagUtility),max(fDefendFlagUtility,fGetFlagUtility)));

		ADD_UTILITY(BOT_UTIL_DEMO_STICKYTRAP_FLAG,
			CTeamFortress2Mod::isMapType(TF_MAP_CTF) && !bHasFlag && 
			(!m_fLastKnownTeamFlagTime || (m_fLastKnownTeamFlagTime < engine->Time())) &&
			canDeployStickies(),
			fDefendFlagUtility+0.3f);

		ADD_UTILITY(BOT_UTIL_DEMO_STICKYTRAP_FLAG_LASTKNOWN,
			(CTeamFortress2Mod::isMapType(TF_MAP_MVM)||CTeamFortress2Mod::isMapType(TF_MAP_CTF)||(CTeamFortress2Mod::isMapType(TF_MAP_SD) && 
		(CTeamFortress2Mod::getFlagCarrierTeam()==CTeamFortress2Mod::getEnemyTeam(iTeam)))) && !bHasFlag && 
			(m_fLastKnownTeamFlagTime && (m_fLastKnownTeamFlagTime > engine->Time())) &&
			canDeployStickies(),fDefendFlagUtility+0.4f);

		ADD_UTILITY(BOT_UTIL_DEMO_STICKYTRAP_POINT,(iTeam==TF2_TEAM_RED)&&(m_iCurrentDefendArea>0) && 
			(CTeamFortress2Mod::isMapType(TF_MAP_MVM)||CTeamFortress2Mod::isMapType(TF_MAP_SD)||CTeamFortress2Mod::isMapType(TF_MAP_CART)||
			CTeamFortress2Mod::isMapType(TF_MAP_CARTRACE)||CTeamFortress2Mod::isMapType(TF_MAP_ARENA)||
			CTeamFortress2Mod::isMapType(TF_MAP_KOTH)||CTeamFortress2Mod::isMapType(TF_MAP_CP)||
			CTeamFortress2Mod::isMapType(TF_MAP_TC)) &&  canDeployStickies(),
			fDefendFlagUtility+0.4f);

		ADD_UTILITY(BOT_UTIL_DEMO_STICKYTRAP_PL,
			(CTeamFortress2Mod::isMapType(TF_MAP_CART)||CTeamFortress2Mod::isMapType(TF_MAP_CARTRACE)) && 
			(m_pDefendPayloadBomb!=NULL) &&  canDeployStickies(),
			fDefendFlagUtility+0.4f);
	}

	//if ( !CTeamFortress2Mod::hasRoundStarted() && (iTeam == TF_TEAM_BLUE) )
	//{
	if ( bot_messaround.GetBool() )
	{
		float fMessUtil = fGetFlagUtility+0.2f;

		if ( getClass() == TF_CLASS_MEDIC )
			fMessUtil -= randomFloat(0.0f,0.3f);

		ADD_UTILITY(BOT_UTIL_MESSAROUND,(getHealthPercent()>0.75f) && ((iTeam==TF2_TEAM_BLUE)||(!CTeamFortress2Mod::isAttackDefendMap())) && !CTeamFortress2Mod::hasRoundStarted(),fMessUtil);
	}
	//}

	/////////////////////////////////////////////////////////
	// Work out utilities
	//////////////////////////////////////////////////////////
	utils.execute();

	while ( (next = utils.nextBest()) != NULL )
	{
		if ( !m_pSchedules->isEmpty() && bCheckCurrent )
		{
			if ( m_CurrentUtil != next->getId() )
				m_pSchedules->freeMemory();
			else
				break;
		} 

		bCheckCurrent = false;

		if ( executeAction(next->getId(),pWaypointResupply,pWaypointHealth,pWaypointAmmo) )
		{
			m_CurrentUtil = next->getId();

			if ( CClients::clientsDebugging(BOT_DEBUG_UTIL) )
			{
				CClients::clientDebugMsg(BOT_DEBUG_UTIL,g_szUtils[next->getId()],this);
			}

			utils.freeMemory();
			return;
		}
	}

	utils.freeMemory();
}


bool CBotTF2 :: canDeployStickies ()
{
	if ( m_pEnemy.get() != NULL )
	{
		if ( CBotGlobals::isAlivePlayer(m_pEnemy) )
		{
			if ( isVisible(m_pEnemy) )
				return false;		
		}
	}

	// enough ammo???
	CBotWeapon *pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_PIPEBOMBS));

	if ( pWeapon )
	{
		return ( pWeapon->getAmmo(this) >= 6 );
	}

	return false;
}

#define STICKY_INIT			0
#define STICKY_SELECTWEAP	1
#define STICKY_RELOAD		2
#define STICKY_FACEVECTOR   3

// returns true when finished
bool CBotTF2 ::deployStickies(eDemoTrapType type, Vector vStand, Vector vLocation, Vector vSpread, Vector *vPoint, int *iState,int *iStickyNum, bool *bFail, float *fTime)
{
	CBotWeapon *pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_PIPEBOMBS));
	int iPipesLeft = 0;

	if ( pWeapon )
	{
		iPipesLeft = pWeapon->getAmmo(this);
	}

	if ( *iState == STICKY_INIT )
	{
		if ( iPipesLeft < 6 )
			*iStickyNum = iPipesLeft;
		else
			*iStickyNum = 6;

		*iState = 1;
	}

	if ( getCurrentWeapon() != pWeapon )
		selectBotWeapon(pWeapon);
	else
	{
		if ( *iState == 1 )
		{
			*vPoint = vLocation + Vector(randomFloat(-vSpread.x,vSpread.x),randomFloat(-vSpread.y,vSpread.y),0);
			*iState = 2;
		}

		if ( distanceFrom(vStand) > 70 )
			setMoveTo(vStand);
		else
			stopMoving();

		if ( *iState == 2 )
		{
			setLookVector(*vPoint);
			setLookAtTask((LOOK_VECTOR));

			if ( (*fTime < engine->Time()) && (CBotGlobals::yawAngleFromEdict(m_pEdict,*vPoint) < 20) )
			{
				primaryAttack();
				*fTime = engine->Time() + randomFloat(1.0f,1.5f);
				*iState = 1;
				*iStickyNum = *iStickyNum - 1;
			}
		}

		if ( (*iStickyNum == 0) || (iPipesLeft==0)  )
		{
			m_iTrapType = type;
			m_vStickyLocation = vLocation;
		}
	}

	return m_iTrapType!=TF_TRAP_TYPE_NONE;
}

void CBotTF2::detonateStickies(bool isJumping)
{
	// don't try to blow myself up unless i'm jumping
	if ( isJumping || (distanceFrom(m_vStickyLocation) > (BLAST_RADIUS/2)) )
	{
		secondaryAttack();
		m_iTrapType = TF_TRAP_TYPE_NONE;
	}
}

bool CBotTF2::lookAfterBuildings ( float *fTime )
{
	CBotWeapon *pWeapon = getCurrentWeapon();

	wantToListen(false);

	setLookAtTask((LOOK_AROUND));

	if ( !pWeapon )
		return false;
	else if ( pWeapon->getID() != TF2_WEAPON_WRENCH )
	{
		if ( !select_CWeapon(CWeapons::getWeapon(TF2_WEAPON_WRENCH)) )
			return false;
	}

	if ( m_pSentryGun )
	{
		if ( m_prevSentryHealth > CClassInterface::getSentryHealth(m_pSentryGun) )
			return true;

		m_prevSentryHealth = CClassInterface::getSentryHealth(m_pSentryGun);

		if ( distanceFrom(m_pSentryGun) > 100 )
			setMoveTo(CBotGlobals::entityOrigin(m_pSentryGun));
		else
		{
			stopMoving();
			
			duck(true); // crouch too

		}

		lookAtEdict(m_pSentryGun);
		setLookAtTask((LOOK_EDICT)); // LOOK_EDICT fix engineers not looking at their sentry

		if ( *fTime < engine->Time() )
		{
			m_pButtons->tap(IN_ATTACK);
			*fTime = engine->Time() + randomFloat(10.0f,20.0f);
		}

	}
	
	if ( m_pDispenser )
	{
		if ( m_prevDispHealth > CClassInterface::getDispenserHealth(m_pDispenser) )
			return true;

		m_prevDispHealth = CClassInterface::getDispenserHealth(m_pDispenser);
	}

	if ( m_pTeleExit )
	{
		if ( m_prevTeleExtHealth > CClassInterface::getTeleporterHealth(m_pTeleExit) )
			return true;

		m_prevTeleExtHealth = CClassInterface::getTeleporterHealth(m_pTeleExit);
	}

	if ( m_pTeleEntrance )
	{
		if ( m_prevTeleEntHealth > CClassInterface::getTeleporterHealth(m_pTeleEntrance) )
			return true;

		m_prevTeleEntHealth = CClassInterface::getTeleporterHealth(m_pTeleEntrance);
	}

	return false;
}

bool CBotTF2 :: select_CWeapon ( CWeapon *pWeapon )
{
	char cmd[128];
	CBotWeapon *pBotWeapon;

	pBotWeapon = m_pWeapons->getWeapon(pWeapon);

	if ( pBotWeapon && !pBotWeapon->hasWeapon() )
		return false;
	if ( pBotWeapon && !pBotWeapon->isMelee() && pBotWeapon->canAttack() && pBotWeapon->outOfAmmo(this) )
		return false;

	sprintf(cmd,"use %s",pWeapon->getWeaponName());

	helpers->ClientCommand(m_pEdict,cmd);

	return true;
}

bool CBotTF2 :: selectBotWeapon ( CBotWeapon *pBotWeapon )
{
	CWeapon *pSelect = pBotWeapon->getWeaponInfo();

	if ( pSelect )
	{
		//int id = pSelect->getWeaponIndex();
		char cmd[128];

		sprintf(cmd,"use %s",pSelect->getWeaponName());

		helpers->ClientCommand(m_pEdict,cmd);

		return true;
	}
	else
		failWeaponSelect();

	return false;
}

//
// Execute a given Action
//
bool CBotTF2 :: executeAction ( eBotAction id, CWaypoint *pWaypointResupply, CWaypoint *pWaypointHealth, CWaypoint *pWaypointAmmo )
{
	static CWaypoint *pWaypoint;
	extern ConVar rcbot_move_dist;

		switch ( id )
		{
		case BOT_UTIL_DEFEND_PAYLOAD_BOMB:
			{
				if ( m_pDefendPayloadBomb )
				{
					pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_CAPPOINT,0,m_iCurrentDefendArea,true,this);

					if ( pWaypoint )
					{
						Vector org1 = pWaypoint->getOrigin();
						Vector org2 = CBotGlobals::entityOrigin(m_pDefendPayloadBomb);

						pWaypoint = CWaypoints::randomWaypointGoalBetweenArea(CWaypointTypes::W_FL_DEFEND,m_iTeam,m_iCurrentDefendArea,true,this,true,&org1,&org2);

						if ( pWaypoint )
						{
							m_pSchedules->add(new CBotDefendSched(pWaypoint->getOrigin()));
							removeCondition(CONDITION_PUSH);
							return true;
						}
					}
				}

				pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_DEFEND,getTeam(),m_iCurrentDefendArea,true,this,true);

				if ( pWaypoint )
				{
					m_pSchedules->add(new CBotDefendSched(pWaypoint->getOrigin()));
					removeCondition(CONDITION_PUSH);
					return true;
				}

				if ( m_pDefendPayloadBomb )
				{
					m_pSchedules->add(new CBotTF2DefendPayloadBombSched(m_pDefendPayloadBomb));
					removeCondition(CONDITION_PUSH);
					return true;
				}
			}
			break;
		case BOT_UTIL_PUSH_PAYLOAD_BOMB:
			{
				if ( m_pPushPayloadBomb )
				{
					m_pSchedules->add(new CBotTF2PushPayloadBombSched(m_pPushPayloadBomb));
					removeCondition(CONDITION_PUSH);
					return true;
				}
			}
			break;
		case BOT_UTIL_ENGI_LOOK_AFTER_SENTRY:
			{
				m_pSchedules->add(new CBotTFEngiLookAfterSentry(m_pSentryGun));			
				return true;
			}
			break;
		case BOT_UTIL_DEFEND_FLAG:
			// use last known flag position
			{
				CWaypoint *pWaypoint = NULL;
				
				pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_DEFEND,getTeam(),0,false,this,true);
				
				//if ( pWaypoint && randomInt(0,1) )
				//	pWaypoint = CWaypoints::getPinchPointFromWaypoint(getOrigin(),pWaypoint->getOrigin());

				if ( pWaypoint )
				{
					setLookAt(pWaypoint->getOrigin());
					m_pSchedules->add(new CBotDefendSched(pWaypoint->getOrigin(),(m_iClass == TF_CLASS_MEDIC) ? randomFloat(5.0f,10.0f) : 0.0f));
					removeCondition(CONDITION_PUSH);
					return true;
				}
			}
			break;
		case BOT_UTIL_DEFEND_FLAG_LASTKNOWN:
			// find our flag waypoint
			{
				setLookAt(m_vLastKnownTeamFlagPoint);
				m_pSchedules->add(new CBotDefendSched(m_vLastKnownTeamFlagPoint,(m_iClass == TF_CLASS_MEDIC) ? randomFloat(5.0f,10.0f) : 0.0f));
				removeCondition(CONDITION_PUSH);
				return true;
			}
			break;
		case BOT_UTIL_ATTACK_POINT:
			
			/*pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_DEFEND,getTeam(),m_iCurrentAttackArea,true);

			if ( pWaypoint )
			{
				m_pSchedules->add(new CBotDefendSched(pWaypoint->getOrigin()));
				return true;
			}*/

			pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_CAPPOINT,0,m_iCurrentAttackArea,true,this);

			if ( pWaypoint )
			{
				CWaypoint *pRoute = NULL;
				Vector vRoute = Vector(0,0,0);
				bool bUseRoute = false;

				if ( (m_fUseRouteTime < engine->Time()) )
				{
				// find random route
					pRoute = CWaypoints::randomRouteWaypoint(this,getOrigin(),pWaypoint->getOrigin(),getTeam(),m_iCurrentAttackArea);

					if ( pRoute )
					{
						bUseRoute = true;
						vRoute = pRoute->getOrigin();
						m_fUseRouteTime = engine->Time() + randomFloat(30.0f,60.0f);
					}
				}

				m_pSchedules->add(new CBotAttackPointSched(pWaypoint->getOrigin(),pWaypoint->getRadius(),pWaypoint->getArea(),bUseRoute,vRoute));
				removeCondition(CONDITION_PUSH);
				return true;
			}
			break;
		case BOT_UTIL_DEFEND_POINT:
			{
				float fprob;

				if ( (CTeamFortress2Mod::isMapType(TF_MAP_CARTRACE) ||  CTeamFortress2Mod::isMapType(TF_MAP_CART)) )
				{
					if ( m_pDefendPayloadBomb !=NULL )
					{
						static const float fSearchDist = 1500.0f;
						Vector vPayloadBomb = CBotGlobals::entityOrigin(m_pDefendPayloadBomb);
						CWaypoint *pCapturePoint = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(vPayloadBomb,fSearchDist,-1,false,false,false,NULL,false,0,true,false,Vector(0,0,0),CWaypointTypes::W_FL_CAPPOINT));

						if ( pCapturePoint )
						{
							float fDistance = pCapturePoint->distanceFrom(vPayloadBomb);

							if ( fDistance == 0 )
								fprob = 1.0f;
							else
								fprob = 1.0f - (fDistance/fSearchDist);
						}
						else // no where near the capture point 
						{
							extern ConVar bot_defrate;

							fprob = bot_defrate.GetFloat();
						}
					}
					else
					{
						fprob = 0.05f;
					}
				}
				else
				{
					float fTime = rcbot_tf2_protect_cap_time.GetFloat();
					// chance of going to point
					fprob = (fTime - (engine->Time() - CPoints::getPointCaptureTime(m_iCurrentDefendArea)))/fTime;

					if ( fprob < rcbot_tf2_protect_cap_percent.GetFloat() )
						fprob = rcbot_tf2_protect_cap_percent.GetFloat();
				}

				if ( randomFloat(0.0,1.0f) > fprob )
				{
					pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_DEFEND,getTeam(),m_iCurrentDefendArea,true,this,false);

					if ( pWaypoint )
					{
						m_pSchedules->add(new CBotDefendSched(pWaypoint->getOrigin(),(m_iClass == TF_CLASS_MEDIC) ? randomFloat(5.0f,10.0f) : 0.0f));
						removeCondition(CONDITION_PUSH);
						return true;
					}
				}

				pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_CAPPOINT,0,m_iCurrentDefendArea,true,this);

				if ( pWaypoint )
				{
					m_pSchedules->add(new CBotDefendPointSched(pWaypoint->getOrigin(),pWaypoint->getRadius(),pWaypoint->getArea()));
					removeCondition(CONDITION_PUSH);
					return true;
				}
			}
			break;
		case BOT_UTIL_CAPTURE_FLAG:
			pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_CAPPOINT,getTeam());

			if ( pWaypoint )
			{
				m_pSchedules->add(new CBotGotoOriginSched(pWaypoint->getOrigin()));
				removeCondition(CONDITION_PUSH);
				return true;
			}
			break;
		case BOT_UTIL_ENGI_DESTROY_ENTRANCE: // destroy and rebuild sentry elsewhere
			engineerBuild(ENGI_ENTRANCE,ENGI_DESTROY);
		case BOT_UTIL_BUILDTELENT:
			pWaypoint = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(m_vTeleportEntrance,300,-1,true,false,true,NULL,false,getTeam(),true));

			if ( pWaypoint )
			{
				Vector vBuild = m_vTeleportEntrance+pWaypoint->applyRadius();

				m_pSchedules->add(new CBotTFEngiBuild(ENGI_ENTRANCE,vBuild,getAiming(),pWaypoint->getArea()));
				m_iTeleEntranceArea = pWaypoint->getArea();
				return true;
			}
			
			break;
		case BOT_UTIL_ENGI_DESTROY_EXIT: // destroy and rebuild sentry elsewhere
			engineerBuild(ENGI_EXIT,ENGI_DESTROY);
		case BOT_UTIL_BUILDTELEXT:

			if ( m_bTeleportExitVectorValid )
				pWaypoint = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(m_vTeleportExit,150,-1,true,false,true,NULL,false,getTeam(),true));
			else
				pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_TELE_EXIT,getTeam(),0,false,this);//CTeamFortress2Mod::getArea());

			if ( pWaypoint )
			{
				m_bTeleportExitVectorValid = true;
				m_vTeleportExit = pWaypoint->getOrigin()+pWaypoint->applyRadius();
				m_pSchedules->add(new CBotTFEngiBuild(ENGI_EXIT,m_vTeleportExit,getAiming(),pWaypoint->getArea()));
				m_iTeleExitArea = pWaypoint->getArea();
				return true;
			}

			break;
		case BOT_UTIL_ENGI_DESTROY_SENTRY: // destroy and rebuild sentry elsewhere
			engineerBuild(ENGI_SENTRY,ENGI_DESTROY);
		case BOT_UTIL_BUILDSENTRY:

			if ( m_bSentryGunVectorValid )
				pWaypoint = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(m_vSentryGun,150,-1,true,false,true,NULL,false,getTeam(),true));
			else
			{
				pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_SENTRY,getTeam(),0,false,this);
			}

			if ( pWaypoint )
			{
				m_vSentryGun = pWaypoint->getOrigin()+pWaypoint->applyRadius();
				m_bSentryGunVectorValid = true;
				m_pSchedules->add(new CBotTFEngiBuild(ENGI_SENTRY,m_vSentryGun,getAiming(),pWaypoint->getArea()));
				m_iSentryArea = pWaypoint->getArea();
				return true;
			}
			break;
		case BOT_UTIL_BACKSTAB:
			if ( m_pEnemy  && CBotGlobals::isAlivePlayer(m_pEnemy) )
			{
				m_pSchedules->add(new CBotBackstabSched(m_pEnemy));
				return true;
			}
			else if ( m_pLastEnemy &&  CBotGlobals::isAlivePlayer(m_pLastEnemy) )
			{
				m_pSchedules->add(new CBotBackstabSched(m_pLastEnemy));
				return true;
			}
			
			break;
		case  BOT_UTIL_REMOVE_TMTELE_SAPPER:
						
			m_pSchedules->add(new CBotRemoveSapperSched(m_pNearestTeleEntrance,ENGI_TELE));
			return true;

		case BOT_UTIL_REMOVE_SENTRY_SAPPER:

			m_pSchedules->add(new CBotRemoveSapperSched(m_pSentryGun,ENGI_SENTRY));
			return true;

		case BOT_UTIL_REMOVE_DISP_SAPPER:

			m_pSchedules->add(new CBotRemoveSapperSched(m_pDispenser,ENGI_DISP));
			return true;

		case BOT_UTIL_REMOVE_TMSENTRY_SAPPER:

			m_pSchedules->add(new CBotRemoveSapperSched(m_pNearestAllySentry,ENGI_SENTRY));
			return true;

			break;
		case BOT_UTIL_REMOVE_TMDISP_SAPPER:

			m_pSchedules->add(new CBotRemoveSapperSched(m_pNearestDisp,ENGI_DISP));
			return true;

			break;
		case BOT_UTIL_ENGI_DESTROY_DISP:
			engineerBuild(ENGI_DISP,ENGI_DESTROY);
		case BOT_UTIL_BUILDDISP:
			pWaypoint = NULL;
			if ( m_bDispenserVectorValid )
				pWaypoint = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(m_vDispenser,150,-1,true,false,true,NULL,false,getTeam(),true));
			else if ( m_pSentryGun.get() != NULL )
				pWaypoint = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(CBotGlobals::entityOrigin(m_pSentryGun),150,-1,true,false,true,NULL,false,getTeam(),true));			

			if ( pWaypoint )
			{
				m_vDispenser = pWaypoint->getOrigin();
				m_bDispenserVectorValid = true;
				m_pSchedules->add(new CBotTFEngiBuild(ENGI_DISP,m_vDispenser+Vector(randomFloat(-96,96),randomFloat(-96,96),0),getAiming(),pWaypoint->getArea()));
				m_iDispenserArea = pWaypoint->getArea();
				return true;
			}
			break;
		case BOT_UTIL_HIDE_FROM_ENEMY:
			{
				CBotSchedule *pSchedule = new CBotSchedule();

				pSchedule->setID(SCHED_GOOD_HIDE_SPOT);

				// run at flank while shooting	
				CFindPathTask *pHideGoalPoint = new CFindPathTask();
				Vector vOrigin = CBotGlobals::entityOrigin(m_pEnemy);
				
				// no interrupts, should be a quick waypoint path anyway
				pHideGoalPoint->setNoInterruptions();
				// get vector from good hide spot task
				pHideGoalPoint->getPassedVector();
				
				pSchedule->addTask(new CFindGoodHideSpot(vOrigin));
				pSchedule->addTask(pHideGoalPoint);
				pSchedule->addTask(new CBotNest());
				//pSchedule->setID(SCHED_HIDE_FROM_ENEMY);

				m_pSchedules->removeSchedule(SCHED_GOOD_HIDE_SPOT);
				m_pSchedules->addFront(pSchedule);

				return true;
			}
		case BOT_UTIL_MEDIC_HEAL:			
			m_pSchedules->add(new CBotTF2HealSched(m_pHeal));
			return true;
		case BOT_UTIL_MEDIC_HEAL_LAST:
			m_pSchedules->add(new CBotTF2HealSched(m_pLastHeal));
			return true;
		case BOT_UTIL_UPGTMSENTRY:
			m_pSchedules->add(new CBotTFEngiUpgrade(m_pNearestAllySentry));
			return true;
		case BOT_UTIL_UPGTMDISP:
			m_pSchedules->add(new CBotTFEngiUpgrade(m_pNearestDisp));
			return true;
		case BOT_UTIL_UPGSENTRY:
			m_pSchedules->add(new CBotTFEngiUpgrade(m_pSentryGun));
			return true;
		case BOT_UTIL_UPGTELENT:
			m_pSchedules->add(new CBotTFEngiUpgrade(m_pTeleEntrance));
			return true;
		case BOT_UTIL_UPGTELEXT:
			m_pSchedules->add(new CBotTFEngiUpgrade(m_pTeleExit));
			return true;
		case BOT_UTIL_UPGDISP:
			m_pSchedules->add(new CBotTFEngiUpgrade(m_pDispenser));
			return true;
		case BOT_UTIL_GETAMMODISP:
			m_pSchedules->add(new CBotGetMetalSched(CBotGlobals::entityOrigin(m_pDispenser)));
			return true;
		case BOT_UTIL_GOTORESUPPLY_FOR_HEALTH:
			m_pSchedules->add(new CBotTF2GetHealthSched(pWaypointResupply->getOrigin()));
			return true;
		case BOT_UTIL_GOTORESUPPLY_FOR_AMMO:
			m_pSchedules->add(new CBotTF2GetAmmoSched(pWaypointResupply->getOrigin()));
			return true;
		case BOT_UTIL_FIND_NEAREST_HEALTH:
			m_pSchedules->add(new CBotTF2GetHealthSched(pWaypointHealth->getOrigin()));
			return true;
		case BOT_UTIL_FIND_NEAREST_AMMO:
			m_pSchedules->add(new CBotTF2GetAmmoSched(pWaypointAmmo->getOrigin()));
			return true;
		case BOT_UTIL_GOTODISP:
			m_pSchedules->removeSchedule(SCHED_USE_DISPENSER);
			m_pSchedules->addFront(new CBotUseDispSched(m_pNearestDisp));

			m_fPickupTime = engine->Time() + randomFloat(6.0f,20.0f);
			return true;
		case BOT_UTIL_ENGI_MOVE_SENTRY:
			if ( m_pSentryGun.get() )
			{
				Vector vSentry = CBotGlobals::entityOrigin(m_pSentryGun);
				CWaypoint *pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_SENTRY,getTeam(),0,false,this);

				if ( pWaypoint && (pWaypoint->distanceFrom(vSentry) > rcbot_move_dist.GetFloat()) )
				{
					m_pSchedules->add(new CBotEngiMoveBuilding(m_pEdict,m_pSentryGun.get(),ENGI_SENTRY,pWaypoint->getOrigin(),m_bIsCarryingSentry));
					m_iSentryArea = pWaypoint->getArea();
					return true;
				}
				//else
				//	destroySentry();
			}
			return false;
		case BOT_UTIL_SPYCHECK_AIR:
			m_pSchedules->add(new CBotSchedule(new CSpyCheckAir()));
			return true;
		case BOT_UTIL_PLACE_BUILDING:
			if ( m_bIsCarryingObj )
			{
				primaryAttack(); // just press attack to place

				/* -- unused 
				eEngiBuild iObject = ENGI_DISP;
				QAngle eyes = eyeAngles();
				Vector vForward;

				AngleVectors(eyes,&vForward);
				vForward = vForward/vForward.Length();

				if ( m_bIsCarryingTeleEnt ) 
					iObject = ENGI_ENTRANCE;
				else if ( m_bIsCarryingTeleExit )
					iObject = ENGI_EXIT;
				else if ( m_bIsCarryingDisp )
					iObject = ENGI_DISP;
				else if ( m_bIsCarryingSentry )	
					iObject = ENGI_SENTRY;


				m_pSchedules->add(new CBotSchedule(new CBotTaskEngiPlaceBuilding(iObject,getOrigin()+vForward*32.0f)));
*/
				return true;
			}
			return false;
		case BOT_UTIL_ENGI_MOVE_DISP:
			if ( m_pSentryGun.get() && m_pDispenser.get() )
			{
				Vector vDisp = CBotGlobals::entityOrigin(m_pDispenser);
				CWaypoint *pWaypoint = NULL;
				
				if ( m_pSentryGun )
					pWaypoint = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(CBotGlobals::entityOrigin(m_pSentryGun),150,-1,true,false,true,NULL,false,getTeam(),true));		
				else
					pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_SENTRY,getTeam(),0,false,this);

				if ( pWaypoint && (pWaypoint->distanceFrom(vDisp) > rcbot_move_dist.GetFloat()) )
				{
					m_pSchedules->add(new CBotEngiMoveBuilding(m_pEdict,m_pDispenser.get(),ENGI_DISP,pWaypoint->getOrigin(),m_bIsCarryingDisp));
					m_iDispenserArea = pWaypoint->getArea();
					return true;
				}
			}
			return false;
		case BOT_UTIL_ENGI_MOVE_ENTRANCE:

			if ( m_pTeleEntrance.get() )
			{
				Vector vTele = CBotGlobals::entityOrigin(m_pTeleEntrance);
				CWaypoint *pWaypoint = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(m_vTeleportEntrance,512,-1,true,false,true,NULL,false,getTeam(),true));

				if ( pWaypoint &&  ( pWaypoint->distanceFrom(vTele) > rcbot_move_dist.GetFloat() ) )
				{
					m_pSchedules->add(new CBotEngiMoveBuilding(m_pEdict,m_pTeleEntrance.get(),ENGI_ENTRANCE, pWaypoint->getOrigin(),m_bIsCarryingTeleEnt));
					m_iTeleEntranceArea = pWaypoint->getArea();
					return true;
				}
			}

			return false;
		case BOT_UTIL_ENGI_MOVE_EXIT:

			if ( m_pTeleExit.get() )
			{
				Vector vTele = CBotGlobals::entityOrigin(m_pTeleExit);
				CWaypoint *pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_TELE_EXIT,getTeam(),0,false,this);//CTeamFortress2Mod::getArea());

				if ( pWaypoint && (pWaypoint->distanceFrom(vTele) > rcbot_move_dist.GetFloat()) )
				{
					m_pSchedules->add(new CBotEngiMoveBuilding(m_pEdict,m_pTeleExit.get(),ENGI_EXIT,pWaypoint->getOrigin(),m_bIsCarryingTeleExit));
					m_iTeleExitArea = pWaypoint->getArea();
					return true;
				}
			}
			return false;
		case BOT_UTIL_FIND_MEDIC_FOR_HEALTH:
			{
				Vector vLoc = m_pLastSeeMedic.getLocation();
				int iWpt = CWaypointLocations::NearestWaypoint(vLoc,400,-1,true,false,true,0,false,getTeam(),true);
				if ( iWpt != -1 )
				{
					CFindPathTask *findpath = new CFindPathTask(iWpt,LOOK_WAYPOINT);
					CTaskVoiceCommand *shoutMedic = new CTaskVoiceCommand(TF_VC_MEDIC);
					CBotTF2WaitHealthTask *wait = new CBotTF2WaitHealthTask(vLoc);
					CBotSchedule *newSched = new CBotSchedule();

					findpath->setCompleteInterrupt(0,CONDITION_NEED_HEALTH);
					shoutMedic->setCompleteInterrupt(0,CONDITION_NEED_HEALTH);
					wait->setCompleteInterrupt(0,CONDITION_NEED_HEALTH);

					newSched->addTask(findpath);
					newSched->addTask(shoutMedic);
					newSched->addTask(wait);
					m_pSchedules->addFront(newSched);

					return true;
				}

				return false;
			}
		case BOT_UTIL_GETHEALTHKIT:
			m_pSchedules->removeSchedule(SCHED_PICKUP);
			m_pSchedules->addFront(new CBotPickupSched(m_pHealthkit));

			m_fPickupTime = engine->Time() + randomFloat(5.0f,10.0f);

			return true;
		case BOT_UTIL_DEMO_STICKYTRAP_LASTENEMY:
		case BOT_UTIL_DEMO_STICKYTRAP_FLAG:
		case BOT_UTIL_DEMO_STICKYTRAP_FLAG_LASTKNOWN:
		case BOT_UTIL_DEMO_STICKYTRAP_POINT:
		case BOT_UTIL_DEMO_STICKYTRAP_PL:
// to do
			{
				
				Vector vStand;
				Vector vPoint;
				Vector vDemoStickyPoint;
				eDemoTrapType iDemoTrapType = TF_TRAP_TYPE_NONE;

				if ( id == BOT_UTIL_DEMO_STICKYTRAP_LASTENEMY )
				{
					pWaypoint =  CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(m_vLastSeeEnemy,400,-1,true,false,true,0,false,getTeam(),true));
					iDemoTrapType = TF_TRAP_TYPE_WPT;
				}
				else if ( id == BOT_UTIL_DEMO_STICKYTRAP_FLAG )
				{
					pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_FLAG,CTeamFortress2Mod::getEnemyTeam(getTeam()));
					iDemoTrapType = TF_TRAP_TYPE_FLAG;
				}
				else if ( id == BOT_UTIL_DEMO_STICKYTRAP_FLAG_LASTKNOWN )
				{
					pWaypoint =  CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(m_vLastKnownFlagPoint,400,-1,true,false,true,0,false,getTeam(),true));
					iDemoTrapType = TF_TRAP_TYPE_FLAG;
				}
				else if ( id == BOT_UTIL_DEMO_STICKYTRAP_POINT )
				{
					pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_CAPPOINT,0,m_iCurrentDefendArea,true);
					iDemoTrapType = TF_TRAP_TYPE_POINT;
				}
				else if ( id == BOT_UTIL_DEMO_STICKYTRAP_PL )
				{
					pWaypoint =  CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint( CBotGlobals::entityOrigin(m_pDefendPayloadBomb),400,-1,true,false,true,0,false,getTeam(),true));
					iDemoTrapType = TF_TRAP_TYPE_PL;
				}
				
				if ( pWaypoint )
				{
					CWaypoint *pStand = NULL;
					CWaypoint *pTemp;
					float fDist = 9999.0f;
					float fClosest = 9999.0f;

					vPoint = pWaypoint->getOrigin();

					dataUnconstArray<int> m_iVisibles;
					dataUnconstArray<int> m_iInvisibles;

					int iWptFrom = CWaypointLocations::NearestWaypoint(vPoint,2048.0,-1,true,true,false,NULL,false,0,false);

		//int m_iVisiblePoints[CWaypoints::MAX_WAYPOINTS]; // make searching quicker

					CWaypointLocations::GetAllVisible(iWptFrom,iWptFrom,vPoint,vPoint,2048.0,&m_iVisibles,&m_iInvisibles);

					for ( int i = 0; i < m_iVisibles.Size(); i ++ )
					{
						if ( m_iVisibles[i] == CWaypoints::getWaypointIndex(pWaypoint) )
							continue;

						pTemp = CWaypoints::getWaypoint(i);

						if ( pTemp->distanceFrom(pWaypoint) < 512 )
						{
							fDist = distanceFrom(pTemp->getOrigin());

							if ( fDist < fClosest )
							{
								fClosest = fDist;
								pStand = CWaypoints::getWaypoint(i);
							}
						}
					}

					if ( !pStand )
					{
						pStand = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(pWaypoint->getOrigin(),400,CWaypoints::getWaypointIndex(pWaypoint),true,false,true,0,false,getTeam(),true));
					}

					if ( pStand )
					{
						vStand = pStand->getOrigin();

						if ( pWaypoint )
						{
							m_pSchedules->add(new CBotTF2DemoPipeTrapSched(iDemoTrapType,vStand,vPoint,Vector(150,150,20)));
							return true;
						}
					}

					m_iVisibles.Destroy();
					m_iInvisibles.Destroy();
				}
			}
			break;
		case  BOT_UTIL_SAP_NEAREST_TELE:
			m_pSchedules->add(new CBotSpySapBuildingSched(m_pNearestEnemyTeleporter,ENGI_TELE));
			return true;
		case BOT_UTIL_SAP_ENEMY_TELE:
			m_pSchedules->add(new CBotSpySapBuildingSched(m_pEnemy,ENGI_TELE));
			return true;
		case BOT_UTIL_SAP_LASTENEMY_TELE:
			m_pSchedules->add(new CBotSpySapBuildingSched(m_pLastEnemy,ENGI_TELE));
			return true;
		case BOT_UTIL_SAP_LASTENEMY_SENTRY:
			m_pSchedules->add(new CBotSpySapBuildingSched(m_pLastEnemySentry,ENGI_SENTRY));
			return true;
		case BOT_UTIL_SAP_ENEMY_SENTRY:
			m_pSchedules->add(new CBotSpySapBuildingSched(m_pEnemy,ENGI_SENTRY));
			return true;
		case BOT_UTIL_SAP_NEAREST_SENTRY:
			m_pSchedules->add(new CBotSpySapBuildingSched(m_pNearestEnemySentry,ENGI_SENTRY));
			return true;
		case BOT_UTIL_PIPE_NEAREST_SENTRY:
		case BOT_UTIL_PIPE_LAST_ENEMY:
		case BOT_UTIL_PIPE_LAST_ENEMY_SENTRY:
			{
				Vector vLoc;
				Vector vEnemy;
				edict_t *pEnemy;

				vLoc = getOrigin();

				if ( id == BOT_UTIL_PIPE_NEAREST_SENTRY )
				{
					pEnemy = m_pNearestEnemySentry;
					vEnemy = CBotGlobals::entityOrigin(m_pNearestEnemySentry);
				}
				else if ( id == BOT_UTIL_PIPE_LAST_ENEMY_SENTRY )
				{
					pEnemy = m_pLastEnemySentry;
					vEnemy = CBotGlobals::entityOrigin(m_pLastEnemySentry);
					vLoc = m_vLastDiedOrigin;
				}
				else
				{
					pEnemy = m_pLastEnemy;
					vEnemy = m_vLastSeeEnemy;
				}

				CWaypoint *pWptBlast = CWaypoints::getWaypoint(CWaypointLocations::NearestBlastWaypoint(CBotGlobals::entityOrigin(pEnemy),vLoc,4096.0,-1,true,true,false,false,getTeam(),false));

				if ( pWptBlast )
				{
					CWaypoint *pWpt = CWaypoints::getWaypoint(CWaypointLocations::NearestBlastWaypoint(vLoc,pWptBlast->getOrigin(),4096.0,CWaypoints::getWaypointIndex(pWptBlast),true,false,true,false,getTeam(),true,1024.0f));

					if ( pWpt )
					{
						CBotTask *findpath = new CFindPathTask(CWaypoints::getWaypointIndex(pWpt));
						CBotTask *pipetask = new CBotTF2DemomanPipeEnemy(
							pWpt->getOrigin(),pWptBlast->getOrigin(),
							getWeapons()->getWeapon(CWeapons::getWeapon(TF2_WEAPON_PIPEBOMBS)),
							vEnemy,pEnemy);
						CBotSchedule *pipesched = new CBotSchedule();

						pipesched->addTask(findpath);
						pipesched->addTask(pipetask);

						m_pSchedules->add(pipesched);

						return true;
					}
				}
			}
			break;
		case BOT_UTIL_GETAMMOKIT:
			m_pSchedules->removeSchedule(SCHED_PICKUP);
			m_pSchedules->addFront(new CBotPickupSched(m_pAmmo));

			m_fPickupTime = engine->Time() + randomFloat(5.0f,10.0f);
			return true;
		case BOT_UTIL_SNIPE:
			pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_SNIPER,getTeam(),0,false,this);

			if ( pWaypoint )
			{
				m_pSchedules->add(new CBotTF2SnipeSched(pWaypoint->getOrigin(),pWaypoint->getAimYaw(),pWaypoint->getArea()));
				return true;
			}
			break;
		case BOT_UTIL_GETFLAG_LASTKNOWN:
			pWaypoint = CWaypoints::getWaypoint(CWaypoints::nearestWaypointGoal(-1,m_vLastKnownFlagPoint,512.0,getTeam()));

			if ( pWaypoint )
			{
				m_pSchedules->add(new CBotTF2FindFlagSched(m_vLastKnownFlagPoint));
				return true;
			}
			break;
		case BOT_UTIL_MEDIC_FINDPLAYER_AT_SPAWN:
			{
				CWaypoint *pWaypoint = NULL;
				
				pWaypoint = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(m_vTeleportEntrance,300,-1,true,false,true,NULL,false,getTeam(),true));

				//if ( pWaypoint && randomInt(0,1) )
				//	pWaypoint = CWaypoints::getPinchPointFromWaypoint(getOrigin(),pWaypoint->getOrigin());

				if ( pWaypoint )
				{
					setLookAt(pWaypoint->getOrigin());
					m_pSchedules->add(new CBotDefendSched(pWaypoint->getOrigin(),randomFloat(10.0f,25.0f)));
					removeCondition(CONDITION_PUSH);
					return true;
				}
			}
			break;
		case BOT_UTIL_MEDIC_FINDPLAYER:
			{
				m_pSchedules->add(new CBotTF2HealSched(m_pLastCalledMedic));
				// roam
				//pWaypoint = CWaypoints::randomWaypointGoal(-1,getTeam(),0,false,this);

				//if ( pWaypoint )
				//{
				//	m_pSchedules->add(new CBotGotoOriginSched(pWaypoint->getOrigin()));
				//	return true;
				//}
			}
			break;
		case BOT_UTIL_MESSAROUND:
			{
				// find a nearby friendly
				int i = 0;
				edict_t *pEdict;
				edict_t *pNearby = NULL;
				float fMaxDistance = 500;
				float fDistance;

				for ( i = 1; i <= CBotGlobals::maxClients(); i ++ )
				{
					pEdict = INDEXENT(i);

					if ( CBotGlobals::entityIsValid(pEdict) )
					{
						if ( CClassInterface::getTeam(pEdict) == getTeam() )
						{
							if ( (fDistance=distanceFrom(pEdict)) < fMaxDistance )
							{
								if ( isVisible(pEdict) )
								{
									// add a little bit of randomness
									if ( !pNearby || randomInt(0,1) )
									{
										pNearby = pEdict;
										fMaxDistance = fDistance;
									}
								}
							}
						}
					}
				}

				if ( pNearby )
				{
					m_pSchedules->add(new CBotTF2MessAroundSched(pNearby,TF_VC_INVALID));
					return true;
				}

				return false;
			}
			break;
		case BOT_UTIL_GETFLAG:
			pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_FLAG,getTeam());

			if ( pWaypoint )
			{
				CWaypoint *pRoute = NULL;
				Vector vRoute = Vector(0,0,0);
				bool bUseRoute = false;

				if ( (m_fUseRouteTime < engine->Time()) )
				{
				// find random route
					pRoute = CWaypoints::randomRouteWaypoint(this,getOrigin(),pWaypoint->getOrigin(),getTeam(),m_iCurrentAttackArea);

					if ( pRoute )
					{
						bUseRoute = true;
						vRoute = pRoute->getOrigin();
						m_fUseRouteTime = engine->Time() + randomFloat(30.0f,60.0f);
					}
				}

				m_pSchedules->add(new CBotTF2GetFlagSched(pWaypoint->getOrigin(),bUseRoute,vRoute));

				return true;
			}
				
			break;
		case BOT_UTIL_ROAM:
			// roam
			pWaypoint = CWaypoints::randomWaypointGoal(-1,getTeam(),0,false,this);

			if ( pWaypoint )
			{
				m_pSchedules->add(new CBotGotoOriginSched(pWaypoint->getOrigin()));
				return true;
			}
			break;
		default:
			break;
		}

		return false;
}

void CBotTF2 :: touchedWpt ( CWaypoint *pWaypoint )
{
	static int wptindex;

	CBot::touchedWpt(pWaypoint);

	if ( canGotoWaypoint (getOrigin(), pWaypoint) )
	{
		if ( pWaypoint->hasFlag(CWaypointTypes::W_FL_ROCKET_JUMP) )
		{
			if ( getNavigator()->hasNextPoint() )
			{
				if ( getClass() == TF_CLASS_SOLDIER )
					m_pSchedules->addFront(new CBotSchedule(new CBotTFRocketJump()));
				else if ( (getClass() == TF_CLASS_DEMOMAN) && rcbot_demo_jump.GetBool() )
					m_pSchedules->addFront(new CBotSchedule(new CBotTF2DemomanPipeJump(this,pWaypoint->getOrigin(),getNavigator()->getNextPoint(),getWeapons()->getWeapon(CWeapons::getWeapon(TF2_WEAPON_PIPEBOMBS)))));
			}
		}
		else if ( pWaypoint->hasFlag(CWaypointTypes::W_FL_DOUBLEJUMP) )
		{
			extern ConVar bot_scoutdj;
			m_pButtons->tap(IN_JUMP);
			m_fDoubleJumpTime = engine->Time() + bot_scoutdj.GetFloat();
			//m_pSchedules->addFront(new CBotSchedule(new CBotTFDoubleJump()));
		}
		else if ( pWaypoint->getFlags() == 0 )
		{
			if ( getNavigator()->hasNextPoint() && (getClass() == TF_CLASS_SCOUT) )
			{
				if ( randomFloat(0.0f,100.0f) > (m_pProfile->m_fBraveness*10) )
				{
					float fVel = m_vVelocity.Length();

					if ( fVel > 0.0f )
					{
						Vector v_next = getNavigator()->getNextPoint();
						Vector v_org = getOrigin();					
						Vector v_comp = v_next-v_org;
						float fDist = v_comp.Length();

						Vector v_vel = (m_vVelocity/fVel) * fDist;
						
						if ( (v_next-(v_org + v_vel)).Length() <= 24.0f )
							m_pButtons->tap(IN_JUMP);
					}
				}
			}
		}
		else if ( pWaypoint->hasFlag(CWaypointTypes::W_FL_FALL) )
		{
			// jump to avoid being hurt (scouts can jump in the air)
			if ( fabs(m_vVelocity.z) > 1 )
				jump();
		}
	}

	// only good for spies so they know when to cloak better
	if ( getClass() == TF_CLASS_SPY )
	{
		wptindex = CWaypoints::getWaypointIndex(pWaypoint);

		if ( m_pEnemy && hasSomeConditions(CONDITION_SEE_CUR_ENEMY) ) 
		{
			m_pNavigator->beliefOne(wptindex,BELIEF_DANGER,distanceFrom(m_pEnemy));
		}
		else
			m_pNavigator->beliefOne(wptindex,BELIEF_SAFETY,0);
	}
}

void CBotTF2 :: modAim ( edict_t *pEntity, Vector &v_origin, Vector *v_desired_offset, Vector &v_size, float fDist )
{
	static CBotWeapon *pWp;
	static float fTime;

	pWp = getCurrentWeapon();
	fDist = distanceFrom(pEntity);

	CBot::modAim(pEntity,v_origin,v_desired_offset,v_size,fDist);

	if ( pWp )
	{
		if ( m_iClass == TF_CLASS_MEDIC )
		{
			if ( pWp->getID() == TF2_WEAPON_SYRINGEGUN )
				v_desired_offset->z += sqrt(fDist)*2;
		}
		else if ( m_iClass == TF_CLASS_HWGUY )
		{
			if ( pWp->getID() == TF2_WEAPON_MINIGUN )
			{
				extern ConVar bot_heavyaimoffset;

				Vector vForward;
				Vector vRight;
				QAngle eyes;

				eyes = eyeAngles();

				// in fov? Check angle to edict
				AngleVectors(eyes,&vForward);
				//vForward = vForward.NormalizeInPlace();

				vRight = vForward.Cross(Vector(0,0,1));

				*v_desired_offset = *v_desired_offset + (((vRight * 24) - Vector(0,0,24))* bot_heavyaimoffset.GetFloat());
			}
		}
		else if ( (m_iClass == TF_CLASS_SOLDIER) || (m_iClass == TF_CLASS_DEMOMAN) )
		{
//			int iSpeed = 0;

			switch ( pWp->getID() )
			{
				case TF2_WEAPON_ROCKETLAUNCHER:
				{
					//iSpeed = TF2_ROCKETSPEED;

					if ( v_origin.z <= getOrigin().z )
						v_desired_offset->z -= randomFloat(8.0f,24.0f);
				}
				// fall through
				case TF2_WEAPON_GRENADELAUNCHER:
				{
					extern ConVar *sv_gravity;
					CClient *pClient = CClients::get(pEntity);
					Vector vVelocity;

					//if ( iSpeed == 0 )
					//	iSpeed = TF2_GRENADESPEED;

					//if ( pClient )
					//{
						if ( CClassInterface :: getVelocity(pEntity,&vVelocity) )
						{
							if ( pClient && (vVelocity == Vector(0,0,0)) )
								vVelocity = pClient->getVelocity();
						}
						else if ( pClient )
							vVelocity = pClient->getVelocity();

						// speed = distance/time
						// .'.
						// time = distance/speed

						if ( pWp->getProjectileSpeed() > 0 )
						{
							fTime = fDist/pWp->getProjectileSpeed();

							*v_desired_offset = *v_desired_offset + ((vVelocity*fTime)*m_pProfile->m_fAimSkill );
						
							if ( (sv_gravity != NULL) && (pWp->getID() == TF2_WEAPON_GRENADELAUNCHER) )
								v_desired_offset->z += (pow(2,fTime)*(sv_gravity->GetFloat()*rcbot_projectile_tweak.GetFloat()));// - (getOrigin().z - v_origin.z);
						}
				}
			break;
			}
		}
	}
}
/*
Vector CBotTF2 :: getAimVector ( edict_t *pEntity )
{
	extern ConVar bot_aimsmoothing;
	
	static CBotWeapon *pWp;
	static float fDist;
	static float fTime;

	pWp = getCurrentWeapon();
	fDist = distanceFrom(pEntity);

	if ( m_fNextUpdateAimVector > engine->Time() )
	{
		//if ( m_bPrevAimVectorValid && bot_aimsmoothing.GetBool() )
		//	return BOTUTIL_SmoothAim(m_vPrevAimVector,m_vAimVector,m_fStartUpdateAimVector,engine->Time(),m_fNextUpdateAimVector);

		return m_vAimVector;
	}

	Vector vAim = CBot::getAimVector(pEntity);

	if ( pWp )
	{

		if ( m_iClass == TF_CLASS_MEDIC )
		{
			if ( pWp->getID() == TF2_WEAPON_SYRINGEGUN )
				vAim = vAim + Vector(0,0,sqrt(fDist)*2);
		}
		else if ( m_iClass == TF_CLASS_HWGUY )
		{
			if ( pWp->getID() == TF2_WEAPON_MINIGUN )
			{
				extern ConVar bot_heavyaimoffset;

				Vector vForward;
				Vector vRight;
				QAngle eyes;

				eyes = eyeAngles();

				// in fov? Check angle to edict
				AngleVectors(eyes,&vForward);
				vForward = vForward.NormalizeInPlace();

				vRight = vForward.Cross(Vector(0,0,1));

				vAim = vAim + (((vRight * 24) - Vector(0,0,24))* bot_heavyaimoffset.GetFloat());
			}
		}
		else if ( (m_iClass == TF_CLASS_SOLDIER) || (m_iClass == TF_CLASS_DEMOMAN) )
		{
			int iSpeed = 0;

			switch ( pWp->getID() )
			{
				case TF2_WEAPON_ROCKETLAUNCHER:
				{
					iSpeed = TF2_ROCKETSPEED;

					if ( vAim.z <= getOrigin().z )
						vAim = vAim - Vector(0,0,randomFloat(8.0f,24.0f));
				}
				// fall through
				case TF2_WEAPON_GRENADELAUNCHER:
				{
					CClient *pClient = CClients::get(pEntity);
					Vector vVelocity;

					if ( iSpeed == 0 )
						iSpeed = TF2_GRENADESPEED;

					if ( pClient )
					{
						if ( CClassInterface :: getVelocity(pEntity,&vVelocity) )
						{
							if ( pClient && (vVelocity == Vector(0,0,0)) )
								vVelocity = pClient->getVelocity();
						}
						else if ( pClient )
							vVelocity = pClient->getVelocity();

						// speed = distance/time
						// .'.
						// time = distance/speed
						fTime = fDist/iSpeed;

						vAim = vAim + ((vVelocity*fTime)*m_pProfile->m_fAimSkill );
					}

					if ( pWp->getID() == TF2_WEAPON_GRENADELAUNCHER )
						vAim = vAim + Vector(0,0,sqrt(fDist));
				}
			break;
			}
		}
	}

	m_fStartUpdateAimVector = engine->Time();
	m_vAimVector = vAim;

	return m_vAimVector;
}
*/
void CBotTF2 :: checkDependantEntities ()
{
	CBotFortress::checkDependantEntities();
	checkEntity(&m_pRedPayloadBomb);
	checkEntity(&m_pBluePayloadBomb);
}

eBotFuncState CBotTF2 :: rocketJump(int *iState,float *fTime)
{
	extern ConVar bot_rj;

	setLookAtTask((LOOK_GROUND));

	switch ( *iState )
	{
	case 0:
		{
			if ( (getSpeed() > 100) && (CBotGlobals::playerAngles(m_pEdict).x > 86.0f )  )
			{
				m_pButtons->tap(IN_JUMP);
				*iState = *iState + 1;
				*fTime = engine->Time() + bot_rj.GetFloat();//randomFloat(0.08,0.5);

				return BOT_FUNC_CONTINUE;
			}
		}
		break;
	case 1:
		{
			if ( *fTime < engine->Time() )
			{
				m_pButtons->tap(IN_ATTACK);

				return BOT_FUNC_COMPLETE;
			}
		}
		break;
	}

	return BOT_FUNC_CONTINUE;
}


// return true if the enemy is ok to shoot, return false if there is a problem (e.g. weapon problem)
bool CBotTF2 :: handleAttack ( CBotWeapon *pWeapon, edict_t *pEnemy )
{
	extern ConVar rcbot_enemyshootfov;
	static float fDistance;

	fDistance = distanceFrom(pEnemy);

	if ( (fDistance > 128) && (DotProductFromOrigin(m_vAimVector) < rcbot_enemyshootfov.GetFloat()) ) 
		return true; // keep enemy / don't shoot : until angle between enemy is less than 45 degrees

	/* Handle Spy Attacking Choice here */
	if ( m_iClass == TF_CLASS_SPY )	 
	{
		if ( isDisguised() )
		{
			if ( ((fDistance < rcbot_tf2_spy_kill_on_cap_dist.GetFloat()) && CTeamFortress2Mod::isCapping(pEnemy)) || 
				( (fDistance < 130) && CBotGlobals::isAlivePlayer(pEnemy) && 
				( fabs(CBotGlobals::yawAngleFromEdict(pEnemy,getOrigin())) > bot_spyknifefov.GetFloat() ) ) )
			{
				; // ok attack
			}
			else if ( m_fFrenzyTime < engine->Time() ) 
				return true; // return but don't attack
			else if ( (CClassInterface::getTF2Class(pEnemy) == TF_CLASS_ENGINEER) && 
						   ( 
						     CTeamFortress2Mod::isMySentrySapped(pEnemy) || 
						     CTeamFortress2Mod::isMyTeleporterSapped(pEnemy) || 
							 CTeamFortress2Mod::isMyDispenserSapped(pEnemy) 
						   )
						)
			{
				return true;  // return but don't attack
			}
		}
	}

	if ( pWeapon )
	{
		Vector vEnemyOrigin;
		bool bSecAttack = false;

		clearFailedWeaponSelect();

		if ( pWeapon->isMelee() )
		{
			setMoveTo(CBotGlobals::entityOrigin(pEnemy));
			//setLookAt(m_vAimVector);
			setLookAtTask((LOOK_ENEMY));
			// dontAvoid my enemy
			m_fAvoidTime = engine->Time() + 1.0f;
		}

		if ( CTeamFortress2Mod::isRocket(m_pEdict,CTeamFortress2Mod::getEnemyTeam(getTeam())) )
		{
			if ( (fDistance < 400) && pWeapon->canDeflectRockets() && (pWeapon->getAmmo(this) > 25) )
				bSecAttack = true;
			else
				return false; // don't attack the rocket anymore
		}

		if ( (m_iClass == TF_CLASS_SNIPER) && (pWeapon->getID() == TF2_WEAPON_SNIPERRIFLE) ) 
		{
			stopMoving();

			if ( m_fSnipeAttackTime < engine->Time() )
			{
				if ( CTeamFortress2Mod::TF2_IsPlayerZoomed(m_pEdict) )
					primaryAttack(); // shoot
				else
					secondaryAttack(); // zoom

				m_fSnipeAttackTime = engine->Time() + randomFloat(0.5f,3.0f);
			}
		}
		else if ( !bSecAttack )
		{
			if ( pWeapon->mustHoldAttack() )
				primaryAttack(true);
			else
				primaryAttack();
		}
		else
		{
			tapButton(IN_ATTACK2);
		}

		vEnemyOrigin = CBotGlobals::entityOrigin(pEnemy);
// enemy below me!
		if ( pWeapon->isMelee() && (vEnemyOrigin.z < (getOrigin().z - 8)) && (vEnemyOrigin.z > (getOrigin().z-128))  )
			duck();

		if ( pWeapon->outOfAmmo(this) )
			return false; // change weapon/enemy
	}
	else
		primaryAttack();

	return true;
}

bool CBotTF2 :: upgradeBuilding ( edict_t *pBuilding, bool removesapper )
{
	Vector vOrigin = CBotGlobals::entityOrigin(pBuilding);

	CBotWeapon *pWeapon = getCurrentWeapon();
	int iMetal = 0;

	wantToListen(false);

	if ( !pWeapon )
		return false;

	iMetal = pWeapon->getAmmo(this);
	
	if ( pWeapon->getID() != TF2_WEAPON_WRENCH )
	{
		if ( !select_CWeapon(CWeapons::getWeapon(TF2_WEAPON_WRENCH)) )
			return false;
	}
	else if ( !removesapper && (iMetal == 0) ) // finished / out of metal // dont need metal to remove sapper
		return true;
	else 
	{	
		clearFailedWeaponSelect();

		if ( distanceFrom(vOrigin) > 85 )
		{
			setMoveTo(vOrigin);			
		}
		else
		{
			duck(true);
			primaryAttack();
		}
	}

	lookAtEdict(pBuilding);
	m_fLookSetTime = 0;
	setLookAtTask((LOOK_EDICT));
	m_fLookSetTime = engine->Time() + randomFloat(3.0,8.0);

	return true;
}

void CBotFortress::teamFlagPickup ()
{
	if ( CTeamFortress2Mod::isMapType(TF_MAP_SD) && m_pSchedules->hasSchedule(SCHED_TF2_GET_FLAG) )
		m_pSchedules->removeSchedule(SCHED_TF2_GET_FLAG); 
}

void CBotTF2::waitRemoveSap ()
{
	// this gives engi bot some time to attack spy that has been sapping a sentry
	m_fRemoveSapTime = engine->Time()+randomFloat(2.5f,4.0f);
	// TO DO :: add spy check task 
}

void CBotTF2::roundReset(bool bFullReset)
{
	m_pRedPayloadBomb = NULL;
	m_pBluePayloadBomb = NULL;

    m_bEntranceVectorValid = false;
	m_bSentryGunVectorValid = false;
	m_bDispenserVectorValid = false;
	m_bTeleportExitVectorValid = false;
	m_pPrevSpy = NULL;
	m_pHeal = NULL;
	m_pSentryGun = NULL;
	m_pDispenser = NULL;
	m_pTeleEntrance = NULL;
	m_pTeleExit = NULL;
	m_pAmmo = NULL;
	m_pHealthkit = NULL;
	m_pNearestDisp = NULL;
	m_pNearestEnemySentry = NULL;
	m_pNearestAllySentry = NULL;
	m_pNearestEnemyTeleporter = NULL;
	m_pFlag = NULL;
	m_pPrevSpy = NULL;
	m_iSentryKills = 0;
	m_fSentryPlaceTime = 0.0;
	m_fDispenserPlaceTime = 0.0f;
	m_fDispenserHealAmount = 0.0f;
	 m_fTeleporterEntPlacedTime = 0.0f;
	 m_fTeleporterExtPlacedTime = 0.0f;
	 m_iTeleportedPlayers = 0;

	flagReset();
	teamFlagReset();

	m_pNavigator->clear();

	// fix : reset current areas
	m_iCurrentDefendArea = 0;
	m_iCurrentAttackArea = 0;

	CPoints::getAreas(getTeam(),&m_iCurrentDefendArea,&m_iCurrentAttackArea);

	//m_pPayloadBomb = NULL;
}

/// TO DO : list of areas
void CBotTF2::getDefendArea ( vector<int> *m_iAreas )
{
	CPoints::getAreas(getTeam(),&m_iCurrentDefendArea,&m_iCurrentAttackArea);
	m_iAreas->push_back(m_iCurrentDefendArea);
}

void CBotTF2::getAttackArea ( vector <int> *m_iAreas )
{
	CPoints::getAreas(getTeam(),&m_iCurrentDefendArea,&m_iCurrentAttackArea);
	m_iAreas->push_back(m_iCurrentAttackArea);
}

void CBotTF2::pointCaptured(int iPoint, int iTeam, const char *szPointName)
{
	extern ConVar rcbot_move_sentry_time;
	extern ConVar rcbot_move_disp_time;
	extern ConVar rcbot_move_tele_time;

	//m_pPayloadBomb = NULL;
	m_pRedPayloadBomb = NULL;
	m_pBluePayloadBomb = NULL;
	if ( m_iClass == TF_CLASS_ENGINEER )
	{
		// think about moving stuff now
		if ( m_pSentryGun.get() && ((m_fSentryPlaceTime + rcbot_move_sentry_time.GetFloat())>engine->Time()) )
			m_fSentryPlaceTime = engine->Time() - rcbot_move_sentry_time.GetFloat();
		if ( m_pDispenser.get() && ((m_fDispenserPlaceTime + rcbot_move_disp_time.GetFloat())>engine->Time()))
			m_fDispenserPlaceTime = engine->Time() - rcbot_move_disp_time.GetFloat();
		if ( m_pTeleEntrance.get() && ((m_fTeleporterEntPlacedTime + rcbot_move_tele_time.GetFloat())>engine->Time()))
			 m_fTeleporterEntPlacedTime = engine->Time() - rcbot_move_tele_time.GetFloat();
		if ( m_pTeleExit.get() && ((m_fTeleporterExtPlacedTime + rcbot_move_tele_time.GetFloat())>engine->Time()))
			 m_fTeleporterExtPlacedTime = engine->Time() - rcbot_move_tele_time.GetFloat();
	}
	m_pNavigator->clear();

	CPoints::getAreas(getTeam(),&m_iCurrentDefendArea,&m_iCurrentAttackArea);
}

// Is Enemy Function
// take a pEdict entity to check if its an enemy
// return TRUE to "OPEN FIRE" (Attack)
// return FALSE to ignore
#define RCBOT_ISENEMY_UNDEF -1
#define RCBOT_ISENEMY_TRUE 1
#define RCBOT_ISENEMY_FALSE 0

bool CBotTF2 :: isEnemy ( edict_t *pEdict,bool bCheckWeapons )
{
	extern ConVar rcbot_notarget;

	static short int iEnemyTeam;
	static bool bIsPipeBomb;
	static bool bIsRocket;
	static int bValid;
	static bool bIsBoss;

	bIsPipeBomb = false;
	bIsRocket = false;
	bValid = false;
	bIsBoss = false;

	if ( !pEdict || !pEdict->GetUnknown() )
		return false;

	if ( !CBotGlobals::entityIsAlive(pEdict) )
		return false;

	if ( rcbot_notarget.GetBool() && (ENTINDEX(pEdict) == 1) )
		return false;

	if ( ENTINDEX(pEdict) <= CBotGlobals::maxClients() )
	{
		if ( CBotGlobals::getTeam(pEdict) != getTeam() )
		{
			if ( m_iClass == TF_CLASS_SPY )	
			{
				if ( !bCheckWeapons )
					return true;
			}	

			if ( CClassInterface::getTF2Class(pEdict) == (int)TF_CLASS_SPY )
			{
				static float fMinReaction;
				static float fMaxReaction;
				static int dteam, dclass, dhealth, dindex;
				static bool bfoundspy;
				static float fSpyAttackTime;
				static edict_t *pDisguisedAs;

				bfoundspy = true; // shout found spy if true

				fSpyAttackTime = engine->Time() - m_fSpyList[ENTINDEX(pEdict)-1];

				if ( CClassInterface::getTF2SpyDisguised(pEdict,&dclass,&dteam,&dindex,&dhealth) )
				{
					pDisguisedAs = (dindex>0)?(INDEXENT(dindex)):(NULL);

					if ( CTeamFortress2Mod::TF2_IsPlayerCloaked(pEdict) ) // if he is cloaked -- can't see him
					{
						bValid = false;

						if ( CTeamFortress2Mod::TF2_IsPlayerOnFire(pEdict) ) // if he is on fire and cloaked I can see him
						{
							// if I saw my team mate shoot him within the last 5 seconds, he's a spy!
							// or no spies on team that can do this!
							bValid = (fSpyAttackTime < 5.0f) || !isClassOnTeam(TF_CLASS_SPY,getTeam());
						}	
					}
					else if ( dteam == 0 ) // not disguised
					{
						bValid = true;
					}
					else if ( dteam != getTeam() )
					{
						bValid = true;
						bfoundspy = false; // disguised as enemy!
					}
					else if ( dindex == ENTINDEX(m_pEdict) ) // if he is disguised as me -- he must be a spy!
					{
						bValid = true;
					}
					else if ( !isClassOnTeam(dclass,getTeam()) ) 
					{// be smart - check if player disguised as a class that exists on my team
						bValid = true;
					}
					else if ( dhealth <= 0 )
					{
						// be smart - check if player's health is below 0
						bValid = true;
					}
					// if he is on fire and I saw my team mate shoot him within the last 5 seconds, he's a spy!
					else if ( CTeamFortress2Mod::TF2_IsPlayerOnFire(pEdict) && (fSpyAttackTime < 5.0f) ) 
					{
						bValid = true;
					}
					// a. I can see the player he is disguised as 
					// b. and the spy was shot recently by a teammate
					// = possibly by the player he was disguised as!
					else if ( pDisguisedAs && isVisible(pDisguisedAs) && (fSpyAttackTime < 5.0f) )
					{
						bValid = true;
					}
					else
						bValid = thinkSpyIsEnemy(pEdict,(TF_Class)dclass);

					if ( bValid && bCheckWeapons && bfoundspy )
						foundSpy(pEdict,(TF_Class)dclass);
				}
				
				//if ( CTeamFortress2Mod::TF2_IsPlayerDisguised(pEdict) || CTeamFortress2Mod::TF2_IsPlayerCloaked(pEdict) )
				//	bValid = false;				
			}
			else
				bValid = true;
		}
	}
	else if ( CTeamFortress2Mod::isBoss(pEdict) )
	{
		bIsBoss = bValid = true;
	}
	// "FrenzyTime" is the time it takes for the bot to check out where he got hurt
	else if ( (m_iClass != TF_CLASS_SPY) || (m_fFrenzyTime > engine->Time()) )	
	{
		iEnemyTeam = CTeamFortress2Mod::getEnemyTeam(getTeam());

		// don't attack sentries if spy, just sap them
		if ( ((m_iClass != TF_CLASS_SPY) && CTeamFortress2Mod::isSentry(pEdict,iEnemyTeam)) || 
			CTeamFortress2Mod::isDispenser(pEdict,iEnemyTeam) || 
			CTeamFortress2Mod::isTeleporter(pEdict,iEnemyTeam) 
			/*CTeamFortress2Mod::isTeleporterExit(pEdict,iEnemyTeam)*/ )
		{
			bValid = true;
		}
		else if ( CTeamFortress2Mod::isPipeBomb ( pEdict, iEnemyTeam ) )
			bIsPipeBomb = bValid = true;
		else if ( CTeamFortress2Mod::isRocket ( pEdict, iEnemyTeam ) )
			bIsRocket = bValid = true;
	}

	if ( bValid )
	{
		if ( bCheckWeapons )
		{
			CBotWeapon *pWeapon = m_pWeapons->getBestWeapon(pEdict);

			if ( pWeapon == NULL )
			{
				return false;
			}
			else
			{
				if ( bIsPipeBomb && !pWeapon->canDestroyPipeBombs() )
					return false;
				else if ( bIsRocket && !pWeapon->canDeflectRockets() )
					return false;
				else if ( bIsBoss && pWeapon->isMelee() )
					return false;
			}
		}

		return true;
	}

	return false;	
}


////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
// FORTRESS FOREVER


void CBotFF :: modThink ()
{
// mod specific think code here
	CBotFortress :: modThink();
}

bool CBotFF :: isEnemy ( edict_t *pEdict,bool bCheckWeapons )
{
	if ( pEdict == m_pEdict )
		return false;

	if ( !ENTINDEX(pEdict) || (ENTINDEX(pEdict) > CBotGlobals::maxClients()) )
		return false;

	if ( CBotGlobals::getTeam(pEdict) == getTeam() )
		return false;

	return true;	
}


// Go back to Cap/Flag to 
void CBotTF2 :: enemyAtIntel ( Vector vPos, int type )
{
	if ( CBotGlobals::entityIsValid(m_pDefendPayloadBomb) && (CTeamFortress2Mod::isMapType(TF_MAP_CART)||CTeamFortress2Mod::isMapType(TF_MAP_CARTRACE)) )
	{
		vPos = CBotGlobals::entityOrigin(m_pDefendPayloadBomb);
	}

	if ( ( m_iClass == TF_CLASS_DEMOMAN ) && ( m_iTrapType != TF_TRAP_TYPE_NONE ) && ( m_iTrapType != TF_TRAP_TYPE_WPT ) )
	{
		detonateStickies();
	}

	m_fRevMiniGunTime = engine->Time()-m_fNextRevMiniGunTime;

	if ( !m_pPlayerInfo )
		return;

	if ( !isAlive() )
		return;
	
	if ( hasFlag() )
		return;

	if ( m_fDefendTime > engine->Time() )
		return;

	if ( m_iClass == TF_CLASS_ENGINEER )
		return; // got work to do...

	// bot is already capturing a point
	if ( m_pSchedules && m_pSchedules->isCurrentSchedule(SCHED_ATTACKPOINT) )
	{
		if ( m_pNavigator && (distanceFrom(m_pNavigator->getGoalOrigin()) < (distanceFrom(vPos)*0.5f)) )
			return;
	}

	if ( type == EVENT_CAPPOINT )
	{
		if ( !m_iCurrentDefendArea )
			return;

		CWaypoint *pWpt = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_CAPPOINT,CTeamFortress2Mod::getEnemyTeam(getTeam()),m_iCurrentDefendArea,true);

		if ( !pWpt )
		{
			return;
		}

		vPos = pWpt->getOrigin();
	}

	// everyone go back to cap point unless doing something important
	if ( (type == EVENT_CAPPOINT) || (!m_pNavigator->hasNextPoint() || ((m_pNavigator->getGoalOrigin()-getOrigin()).Length() > ((vPos-getOrigin()).Length()))) )
	{
		dataUnconstArray<int> *failed;
		m_pNavigator->getFailedGoals(&failed);
		CWaypoint *pWpt = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(vPos,400,-1,true,false,true,failed,false,getTeam(),true));

		if ( pWpt )
		{
			m_pSchedules->freeMemory();
			m_pSchedules->add(new CBotGotoOriginSched(pWpt->getOrigin()));
			m_fDefendTime = engine->Time() + randomFloat(10.0f,20.0f);
		}
	}
	
}

void CBotTF2 :: buildingSapped ( eEngiBuild building, edict_t *pSapper, edict_t *pSpy )
{
	m_pSchedules->freeMemory();

	if ( isVisible(pSpy) )
	{
		foundSpy(pSpy,CTeamFortress2Mod::getSpyDisguise(pSpy));
	}
}

void CBotTF2 :: sapperDestroyed ( edict_t *pSapper )
{
	m_pSchedules->freeMemory();
}

CBotTF2::CBotTF2() 
{ 
		CBotFortress(); 

		m_fDispenserPlaceTime = 0.0f;
		m_fDispenserHealAmount = 0.0f;
	 m_fTeleporterEntPlacedTime = 0;
	 m_fTeleporterExtPlacedTime = 0;
	 m_iTeleportedPlayers = 0;
		m_fDoubleJumpTime = 0;
		m_fSpySapTime = 0;
		m_iCurrentDefendArea = 0;
		m_iCurrentAttackArea = 0;
	    //m_bBlockPushing = false;
	    //m_fBlockPushTime = 0;
		m_pDefendPayloadBomb = NULL;
		m_pPushPayloadBomb = NULL;
		m_pRedPayloadBomb = NULL;
		m_pBluePayloadBomb = NULL;
		m_bFixWeapons = false;
		m_iTrapType = TF_TRAP_TYPE_NONE;
		m_pLastEnemySentry = MyEHandle(NULL);
		m_prevSentryHealth = 0;
		m_prevDispHealth = 0;
		m_prevTeleExtHealth = 0;
		m_prevTeleEntHealth = 0;
		m_fHealClickTime = 0;
		m_fCheckHealTime = 0;
		
		m_fAttackPointTime  = 0; // used in cart maps

		m_prevSentryHealth = 0;
		m_prevDispHealth = 0;
		m_prevTeleExtHealth = 0;
		m_prevTeleEntHealth = 0;

		m_iSentryArea = 0;
		m_iDispenserArea = 0;
		m_iTeleEntranceArea = 0;
		m_iTeleExitArea = 0;

		for ( unsigned int i = 0; i < 10; i ++ )
			m_fClassDisguiseFitness[i] = 1.0f;

		memset(m_fClassDisguiseTime,0,sizeof(float)*10);
}

void CBotTF2 ::init(bool bVarInit)
{
	if( bVarInit )
	{
		CBotTF2();
	}

	CBotFortress::init(bVarInit);
}


