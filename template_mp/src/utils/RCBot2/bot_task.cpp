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
#include "bot_mtrand.h"
#include "bot.h"
#include "bot_schedule.h"
#include "bot_task.h"
#include "bot_navigator.h"
#include "bot_waypoint_locations.h"
#include "bot_globals.h"
#include "in_buttons.h"
#include "bot_weapons.h"
#include "bot_hldm_bot.h"
#include "bot_fortress.h"
#include "bot_profiling.h"
#include "bot_getprop.h"
#include "bot_dod_bot.h"
#include "bot_script.h"

extern ConVar *sv_gravity;
////////////////////////////////////////////////////////////////////////////////////////////
// Tasks

CBotTF2MedicHeal :: CBotTF2MedicHeal ()
{
	m_pHeal = NULL;
}

void CBotTF2MedicHeal::execute(CBot *pBot,CBotSchedule *pSchedule)
{	
	edict_t *pHeal;
	CBotTF2 *pBotTF2;

	pBot->wantToShoot(false);

	if ( !pBot->isTF2() )
	{
		fail();
		return;
	}

	pBotTF2 = (CBotTF2*)pBot;

	pHeal = pBotTF2->getHealingEntity();

	if ( !pHeal )
	{
		// because the medic would have followed this guy, he would have lost his own waypoint
		pBot->getNavigator()->rollBackPosition();
		fail();
	}
	else if ( pBot->getCurrentWeapon() == NULL )
	{
		pBotTF2->clearHealingEntity();
		pBot->getNavigator()->rollBackPosition();
		fail();
	}
	else if ( !pBotTF2->wantToHeal(pHeal) )
	{
		pBotTF2->clearHealingEntity();
		pBot->getNavigator()->rollBackPosition();
		fail();
	}/*
	else if ( !pBot->isVisible(pHeal) )
	{
		pBot->getNavigator()->rollBackPosition();
		((CBotFortress*)pBot)->clearHealingEntity();
		fail();
	}*/
	else if ( pBot->distanceFrom(pHeal) > 416 )
	{
		pBotTF2->clearHealingEntity();
		pBot->getNavigator()->rollBackPosition();
		fail();
	}
	else if ( pBot->getCurrentWeapon()->getWeaponInfo()->getID() != TF2_WEAPON_MEDIGUN )
	{
		pBot->select_CWeapon( CWeapons::getWeapon(TF2_WEAPON_MEDIGUN) );
	}
	else 
	{
		pBot->clearFailedWeaponSelect();

		if ( !pBotTF2->healPlayer(pHeal,m_pHeal) )
		{
			pBot->getNavigator()->rollBackPosition();
			pBotTF2->clearHealingEntity();
			fail();
		}
	}

	m_pHeal = pHeal;
}

///////////


CBotTF2ShootLastEnemyPosition :: CBotTF2ShootLastEnemyPosition  ( Vector vPosition, edict_t *pEnemy, Vector m_vVelocity )
{
	float len = m_vVelocity.Length();

	m_vPosition = vPosition;

	if ( len > 0 )
		m_vPosition = m_vPosition - ((m_vVelocity/m_vVelocity.Length())*16);
	
	m_pEnemy = pEnemy;
	m_fTime = 0;
}

void CBotTF2ShootLastEnemyPosition ::  execute (CBot *pBot,CBotSchedule *pSchedule)
{
	CBotWeapon *pWeapon = pBot->getCurrentWeapon();
	CBotTF2 *pTF2Bot = (CBotTF2*)pBot;
	CWeapon *pChange = NULL;
	CBotWeapon *pChangeTo = NULL;

	if ( m_fTime == 0 )
		m_fTime = engine->Time() + randomFloat(2.0f,4.5f);

	if ( m_fTime < engine->Time() )
	{
		complete();
		return;
	}

	if ( !CBotGlobals::entityIsValid(m_pEnemy) || !CBotGlobals::entityIsAlive(m_pEnemy) )
	{
		complete();
		return;
	}

	if ( pBot->getEnemy() && pBot->hasSomeConditions(CONDITION_SEE_CUR_ENEMY) )
	{
		fail();
		return;
	}

	pBot->wantToShoot(false);
	pBot->wantToChangeWeapon(false);
	pBot->wantToListen(false);

	if ( pTF2Bot->getClass()  == TF_CLASS_SOLDIER )
	{
		pChange = CWeapons::getWeapon(TF2_WEAPON_ROCKETLAUNCHER);
	}
	else if ( pTF2Bot->getClass() == TF_CLASS_DEMOMAN )
	{
		pChange = CWeapons::getWeapon(TF2_WEAPON_GRENADELAUNCHER);
	}

	if ( !pChange )
	{
		fail();
		return;
	}

	pChangeTo = pBot->getWeapons()->getWeapon(pChange);
	
	if ( pChangeTo->getAmmo(pBot) < 1 )
	{
		complete();
		return;
	}

	if ( pChangeTo != pWeapon )
	{
		pBot->selectBotWeapon(pChangeTo);
	}
	else
	{
		if ( randomInt(0,1) )
			pBot->primaryAttack(false);
	}

	pBot->setLookVector(m_vPosition);
	pBot->setLookAtTask((LOOK_VECTOR));

}

void CBotTF2ShootLastEnemyPosition :: debugString ( char *string )
{
	sprintf(string,"CBotTF2ShootLastEnemyPosition\nm_vPosition = (%0.4f,%0.4f,%0.4f)",m_vPosition.x,m_vPosition.y,m_vPosition.z);
}


/////////////

CBotTF2WaitHealthTask :: CBotTF2WaitHealthTask ( Vector vOrigin )
{
	m_vOrigin = vOrigin;
	m_fWaitTime = 0;
}

void CBotTF2WaitHealthTask :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( !m_fWaitTime )
		m_fWaitTime = engine->Time() + randomFloat(5.0f,10.0f);

	 if ( !pBot->hasSomeConditions(CONDITION_NEED_HEALTH) )
		complete();
	 else if ( m_fWaitTime < engine->Time() )
		 fail();
	else
	{
		// TO DO
		/*edict_t *pOtherPlayer = CBotGlobals::findNearestPlayer(m_vOrigin,50.0,pBot->getEdict());

		if ( pOtherPlayer )
		{
			fail();
			return;
		}*/

		pBot->setLookAtTask(LOOK_AROUND);

		if ( pBot->distanceFrom(m_vOrigin) > 50 )
			pBot->setMoveTo((m_vOrigin));
		else
			pBot->stopMoving();

		if ( pBot->isTF() )
		{
			((CBotTF2*)pBot)->taunt();

			if ( ((CBotTF2*)pBot)->isBeingHealed() )
				complete();
		}
	}
}

void CBotTF2WaitHealthTask :: debugString ( char *string )
{
	sprintf(string,"CBotTF2WaitHealthTask\nm_vOrigin = (%0.4f,%0.4f,%0.4f)",m_vOrigin.x,m_vOrigin.y,m_vOrigin.z);
}


CBotTF2WaitFlagTask :: CBotTF2WaitFlagTask ( Vector vOrigin, bool bFind )
{
	m_vOrigin = vOrigin;
	m_fWaitTime = 0;
	m_bFind = bFind;
}

void CBotTF2WaitFlagTask :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( !m_fWaitTime )
	{
		if ( m_bFind )
			m_fWaitTime = engine->Time() + 5.0f;
		else
			m_fWaitTime = engine->Time() + 10.0f;
	}

	if ( ((CBotTF2*)pBot)->hasFlag() )
		complete();
	else if ( pBot->getHealthPercent() < 0.2 )
	{
		fail();
	}
	else if ( m_fWaitTime < engine->Time() )
	{
		((CBotFortress*)pBot)->flagReset();
		fail();
	}
	else if ( !pBot->isTF() )
	{
		fail();
	}
	else
	{		
		if ( !((CBotFortress*)pBot)->waitForFlag(&m_vOrigin,&m_fWaitTime,m_bFind) )
		{
			fail();
		}
	}
}

void CBotTF2WaitFlagTask :: debugString ( char *string )
{
	sprintf(string,"CBotTF2WaitFlagTask\nm_vOrigin = (%0.4f,%0.4f,%0.4f)",m_vOrigin.x,m_vOrigin.y,m_vOrigin.z);
}
//////////
CBotDODBomb :: CBotDODBomb ( int iBombType, int iBombID, edict_t *pBomb, Vector vPosition, int iPrevOwner )
{
	m_iType = iBombType;
	m_iBombID = iBombID; 
	m_fTime = 0;

	if ( m_iBombID == -1 )
		m_iBombID = CDODMod::m_Flags.getBombID(pBomb);

	m_pBombTarget = pBomb;
	m_vOrigin = vPosition;
	m_iPrevTeam = iPrevOwner;
}

void CBotDODBomb :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	bool bWorking = false;

	pBot->wantToShoot(false);

	if ( m_fTime == 0 )
	{
		m_fTime = engine->Time() + randomFloat(8.0f,12.0f);
		((CDODBot*)pBot)->setNearestBomb(m_pBombTarget);

		if ( (m_iType == DOD_BOMB_PLANT) || (m_iType == DOD_BOMB_PATH_PLANT) )
		{
			 if ( CDODMod::m_Flags.isTeamMatePlanting(pBot->getEdict(),pBot->getTeam(),m_iBombID) )
				 fail();
		}
		else
		{
			 if ( CDODMod::m_Flags.isTeamMateDefusing(pBot->getEdict(),pBot->getTeam(),m_iBombID) )
				 fail();
		}
	}
	else if ( m_fTime < engine->Time() )
	{
		fail();
	}

	if ( m_iType == DOD_BOMB_PLANT) 
	{
		bWorking = CClassInterface::isPlayerPlantingBomb_DOD(pBot->getEdict());

		if ( CDODMod::m_Flags.isBombPlanted(m_iBombID) )
		{
			complete();
		}
		else if ( CDODMod::m_Flags.isTeamMatePlanting(pBot->getEdict(),pBot->getTeam(),m_iBombID) )
			complete(); // team mate doing my job
			
		//else if ( !CClassInterface::isPlayerPlantingBomb_DOD(pBot->getEdict()) )// it is still planted
		//	complete(); // bomb is being defused by someone else - give up
	}
	else if ( m_iType == DOD_BOMB_DEFUSE)
	{
		bWorking = CClassInterface::isPlayerDefusingBomb_DOD(pBot->getEdict());

		if ( !CDODMod::m_Flags.isBombPlanted(m_iBombID) )
			complete();
		else if ( CDODMod::m_Flags.isTeamMateDefusing(pBot->getEdict(),pBot->getTeam(),m_iBombID) )
			complete(); // team mate doing my job

		//else if ( CDODMod::m_Flags.isBombBeingDefused(m_iBombID) && !CClassInterface::isPlayerDefusingBomb_DOD(pBot->getEdict()) )// it is still planted
		//	complete(); // bomb is being defused by someone else - give up
	}
	else if ( m_iType == DOD_BOMB_PATH_PLANT ) // a bomb that's in the way 
	{
		bWorking = CClassInterface::isPlayerPlantingBomb_DOD(pBot->getEdict());

		if ( CClassInterface::getDODBombState(m_pBombTarget) != DOD_BOMB_STATE_AVAILABLE )
		{
			//CDODBot *pDODBot = (CDODBot*)pBot;

			//pDODBot->removeBomb();

			complete();
		}
		else if ( CDODMod::m_Flags.isTeamMatePlanting(pBot->getEdict(),pBot->getTeam(),CBotGlobals::entityOrigin(m_pBombTarget)) )
			complete(); // team mate doing my job

	}
	else if ( m_iType == DOD_BOMB_PATH_DEFUSE ) // a bomb that's in the way 
	{
		bWorking = CClassInterface::isPlayerDefusingBomb_DOD(pBot->getEdict());

		if ( CClassInterface::getDODBombState(m_pBombTarget) == DOD_BOMB_STATE_AVAILABLE )
			complete();
		else if ( CDODMod::m_Flags.isTeamMateDefusing(pBot->getEdict(),pBot->getTeam(),CBotGlobals::entityOrigin(m_pBombTarget)) )
			complete(); // team mate doing my job
	}

	pBot->setLookVector(m_vOrigin);
	pBot->setLookAtTask(LOOK_VECTOR);

	pBot->use();
	
	if ( !bWorking )
	{
		pBot->setMoveTo(m_vOrigin);
		pBot->setMoveSpeed(CClassInterface::getMaxSpeed(pBot->getEdict())/4);
	}
	else
	{
		pBot->stopMoving();
	}
}

void CBotDODBomb :: debugString ( char *string )
{
	sprintf(string,"CBotDODBomb\nm_iType = %d\nm_iBombID = %d\nm_fTime = %0.2f\nm_iPrevTeam = %d",m_iType, m_iBombID,m_fTime,m_iPrevTeam);
}

//////


void CDODWaitForBombTask :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( m_fTime == 0.0f )
	{
		CWaypoint *pCurrent;
		pBot->updateCondition(CONDITION_RUN);
		pCurrent = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(pBot->getOrigin(),400.0f,CWaypoints::getWaypointIndex(m_pBlocking),true,false,true));

		if ( pCurrent == NULL )
			pCurrent = m_pBlocking;

		m_fTime = engine->Time() + randomFloat(2.0f,5.0f);
		m_pRunTo = CWaypoints::getNextCoverPoint(pBot,pCurrent,m_pBlocking) ;
	}

	if ( m_pBombTarget.get() == NULL )
	{
		complete();
		return;
	}

	if ( m_pBombTarget.get()->GetUnknown() == NULL )
	{
		complete();
		return;
	}

	pBot->updateCondition(CONDITION_RUN);

	if ( m_pRunTo )
	{
		if (m_pRunTo->touched(pBot->getOrigin(),Vector(0,0,0),48.0f) )
		{
			if ( pBot->distanceFrom(m_pBombTarget) > (BLAST_RADIUS*2) )
				pBot->stopMoving();
			else
				m_pRunTo = CWaypoints::getNextCoverPoint(pBot,m_pRunTo,m_pBlocking) ;
		}
		else
			pBot->setMoveTo(m_pRunTo->getOrigin());
	}

	if ( m_fTime < engine->Time() )
	{
		complete();
		return;
	}

	if ( CClassInterface::getDODBombState (m_pBombTarget) != 2 )
	{
		complete();
		return;
	}

	pBot->lookAtEdict(m_pBombTarget);
	pBot->setLookAtTask(LOOK_EDICT);
}

void CDODWaitForBombTask :: debugString ( char *string )
{
	sprintf(string,"CDODWaitForBombTask");
}

//////////

CBotDODAttackPoint :: CBotDODAttackPoint ( int iFlagID, Vector vOrigin, float fRadius )
{
	m_vOrigin = vOrigin;
	m_fAttackTime = 0;
	m_fTime = 0;
	m_iFlagID = iFlagID;
	m_fRadius = fRadius;
}

void CBotDODAttackPoint :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	static int iTeam;

	iTeam = pBot->getTeam();

	if ( CDODMod::m_Flags.ownsFlag(m_iFlagID,iTeam) )
	{
		complete();
		return;
	}
	else if ( m_fAttackTime == 0 )
		m_fAttackTime = engine->Time() + randomFloat(30.0,60.0);
	else if ( m_fAttackTime < engine->Time() )
	{
		complete();
		return;
	}
	else
	{
		if ( m_fTime == 0 )
		{
			m_fTime = engine->Time() + randomFloat(2.0,4.0);
			m_vMoveTo = m_vOrigin + Vector(randomFloat(-m_fRadius,m_fRadius),randomFloat(-m_fRadius,m_fRadius),0);
			m_bProne = (randomFloat(0,1) * (1.0f-pBot->getHealthPercent())) > 0.75f;

			if (  CDODMod::m_Flags.numFriendliesAtCap(m_iFlagID,iTeam) < CDODMod::m_Flags.numCappersRequired(m_iFlagID,iTeam) )
			{
				// count players I see
				CDODBot *pDODBot = (CDODBot*)pBot;

				pDODBot->addVoiceCommand(DOD_VC_NEED_BACKUP);
			}
		}
		else if ( m_fTime < engine->Time() )
		{
			m_fTime = 0;
		}
		else
		{
			static float fdist;

			fdist = pBot->distanceFrom(m_vMoveTo);

			if ( m_bProne && !pBot->hasSomeConditions(CONDITION_RUN) )
				pBot->duck();

			if ( fdist < m_fRadius )
			{
				pBot->stopMoving();
			}
			else if ( fdist > 400 )
				fail();
			else
			{				
				pBot->setMoveTo(m_vMoveTo);
			}

			pBot->setLookAtTask((LOOK_AROUND));

			if ( ((CBotTF2*)pBot)->checkAttackPoint() )
				complete();
		}
	}
}

void CBotDODAttackPoint :: debugString ( char *string )
{
	sprintf(string,"CBotDODAttackPoint\nm_iFlagID = %d\n m_vOrigin = (%0.1f,%0.1f,%0.1f,radius = %0.1f)",m_iFlagID,m_vOrigin.x,m_vOrigin.y,m_vOrigin.z,m_fRadius);
}

///////////

CBotTF2AttackPoint :: CBotTF2AttackPoint ( int iArea, Vector vOrigin, int iRadius )
{
	m_vOrigin = vOrigin;
	m_fAttackTime = 0;
	m_fTime = 0;
	m_iArea = iArea;
	m_iRadius = iRadius;
}

void CBotTF2AttackPoint :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	vector<int> areas;
	unsigned int i;
	bool found = false;
	CBotTF2 *pTF2Bot = (CBotTF2*)pBot;

	pTF2Bot->getAttackArea(&areas);
	
	i = 0;

	while ( (i < areas.size()) && !found )
	{
		found = areas[i] == m_iArea;
		i++;
	}

	if ( !found )	
		complete();
	else if ( m_fAttackTime == 0 )
		m_fAttackTime = engine->Time() + randomFloat(30.0,60.0);
	else if ( m_fAttackTime < engine->Time() )
		complete();
	else
	{
		if ( m_fTime == 0 )
		{
			
			m_fTime = engine->Time() + randomFloat(5.0,10.0);
			m_vMoveTo = m_vOrigin + Vector(randomFloat(-m_iRadius,m_iRadius),randomFloat(-m_iRadius,m_iRadius),0);
		}
		else if ( m_fTime < engine->Time() )
		{
			m_fTime = 0;
		}
		else
		{
			static float fdist;

			fdist = pBot->distanceFrom(m_vMoveTo);

			if ( pTF2Bot->getClass() == TF_CLASS_SPY )
			{
				if ( pTF2Bot->isDisguised() )
					pBot->primaryAttack(); // remove disguise to capture

				pTF2Bot->wantToDisguise(false);

				// block cloaking
				if ( pTF2Bot->isCloaked() )
				{
					// uncloak
					pBot->secondaryAttack();
				}
				else 
				{
					pBot->letGoOfButton(IN_ATTACK2);
				}

				pTF2Bot->waitCloak();

			}

			if ( fdist < 52 )
			{
				pBot->stopMoving();
			}
			else if ( fdist > 400 )
				fail();
			else
			{				
				pBot->setMoveTo((m_vMoveTo));
			}

			pBot->setLookAtTask((LOOK_AROUND));

			if ( ((CBotTF2*)pBot)->checkAttackPoint() )
				complete();
		}
	}
}

void CBotTF2AttackPoint :: debugString ( char *string )
{
	sprintf(string,"CBotTF2AttackPoint (%d,%0.1f,%0.1f,%0.1f,%d)",m_iArea,m_vOrigin.x,m_vOrigin.y,m_vOrigin.z,m_iRadius);
}

////////////////////////
void CPrimaryAttack:: execute ( CBot *pBot, CBotSchedule *pSchedule )
{
	pBot->primaryAttack();
	complete();
}

////////////////////////////

CBotTF2PushPayloadBombTask :: CBotTF2PushPayloadBombTask (edict_t * pPayloadBomb)
{
	m_pPayloadBomb = pPayloadBomb;
	m_fPushTime = 0;
	m_fTime = 0;
	m_vRandomOffset = Vector(0,0,0);
}

void CBotTF2PushPayloadBombTask :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( m_fPushTime == 0 )
	{
		m_fPushTime = engine->Time() + randomFloat(10.0,30.0);
		m_vRandomOffset = Vector(randomFloat(-50,50),randomFloat(-50,50),0);
	}
	else if ( m_fPushTime < engine->Time() )
	{
		complete();
	}
	else if(m_pPayloadBomb == NULL)
	{
		complete();
		return;
	}

	else if(CBotGlobals::entityIsValid(m_pPayloadBomb) && CBotGlobals::entityIsAlive(m_pPayloadBomb))
	{

		m_vOrigin = CBotGlobals::entityOrigin(m_pPayloadBomb);
		//m_vMoveTo = m_vOrigin + Vector(randomFloat(-10,10),randomFloat(-10,10),0);
		m_vMoveTo = m_vOrigin + m_vRandomOffset;

		if ( pBot->distanceFrom(m_vMoveTo) < 100 )
		{	
			if ( (((CBotTF2*)pBot)->getClass() == TF_CLASS_SPY) && (((CBotTF2*)pBot)->isDisguised()))
				pBot->primaryAttack(); // remove disguise to capture

			((CBotFortress*)pBot)->wantToDisguise(false);

		}
		else
			pBot->setMoveTo((m_vMoveTo));

		pBot->setLookAtTask((LOOK_AROUND));
	}
	else
		complete();

}

void CBotTF2PushPayloadBombTask :: debugString ( char *string )
{
	sprintf(string,"CBotTF2PushPayloadBombTask (%0.1f,%0.1f,%0.1f)",m_vOrigin.x,m_vOrigin.y,m_vOrigin.z);
}
////////////////////////////////////////////////////////////////////////

CBotTF2DefendPayloadBombTask :: CBotTF2DefendPayloadBombTask (edict_t * pPayloadBomb)
{
	m_pPayloadBomb = pPayloadBomb;
	m_fDefendTime = 0;
	m_fTime = 0;
	m_vRandomOffset = Vector(0,0,0);
}

void CBotTF2DefendPayloadBombTask :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( m_fDefendTime == 0 )
	{
		m_fDefendTime = engine->Time() + randomFloat(10.0f,30.0f);
		m_vRandomOffset = Vector(randomFloat(-150.0f,150.0f),randomFloat(-150.0f,150.0f),0);
	}
	else if ( m_fDefendTime < engine->Time() )
	{
		complete();
	}
	else if(m_pPayloadBomb == NULL)
	{
		complete();
		return;
	}
	else if(CBotGlobals::entityIsValid(m_pPayloadBomb) && CBotGlobals::entityIsAlive(m_pPayloadBomb))
	{
		m_vOrigin = CBotGlobals::entityOrigin(m_pPayloadBomb);
		m_vMoveTo = m_vOrigin + m_vRandomOffset;

		if ( pBot->distanceFrom(m_vMoveTo) > 200 )
			pBot->setMoveTo((m_vMoveTo));
		else
			pBot->stopMoving();

		pBot->setLookAtTask((LOOK_EDICT));
		pBot->lookAtEdict(m_pPayloadBomb);
	}
	else
	{
		complete();
	}
}

void CBotTF2DefendPayloadBombTask :: debugString ( char *string )
{
	sprintf(string,"CBotTF2DefendPayloadBombTask (%0.1f,%0.1f,%0.1f)",m_vOrigin.x,m_vOrigin.y,m_vOrigin.z);
}
//////////////////////
CBotTF2DefendPoint :: CBotTF2DefendPoint ( int iArea, Vector vOrigin, int iRadius )
{
	m_vOrigin = vOrigin;
	m_fDefendTime = 0;
	m_fTime = 0;
	m_iArea = iArea;
	m_iRadius = iRadius;
}

void CBotTF2DefendPoint :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	vector<int> areas;
	unsigned int i;
	bool found = false;

	((CBotTF2*)pBot)->getDefendArea(&areas);
	
	i = 0;

	while ( (i < areas.size()) && !found )
	{
		found = areas[i] == m_iArea;
		i++;
	}

	if ( !found )
		complete();
	else if ( m_fDefendTime == 0 )
	{
		m_fDefendTime = engine->Time() + randomFloat(30.0,60.0);
		pBot->resetLookAroundTime();
	}
	else if ( m_fDefendTime < engine->Time() )
		complete();
	else
	{
		if ( m_fTime == 0 )
		{
			float fdist;

			m_fTime = engine->Time() + randomFloat(5.0,10.0);
			m_vMoveTo = m_vOrigin + Vector(randomFloat(-m_iRadius,m_iRadius),randomFloat(-m_iRadius,m_iRadius),0);
			fdist = pBot->distanceFrom(m_vMoveTo);

			if ( fdist < 32 )
				pBot->stopMoving();
			else if ( fdist > 400 )
				fail();
			else
			{				
				pBot->setMoveTo((m_vMoveTo));
			}		
		}
		else if ( m_fTime < engine->Time() )
		{
			m_fTime = 0;
		}
		pBot->setLookAtTask((LOOK_SNIPE));
	}
}

void CBotTF2DefendPoint :: debugString ( char *string )
{
	sprintf(string,"CBotTF2DefendPoint\nm_iArea=%d\nm_vOrigin=(%0.1f,%0.1f,%0.1f,%d)",m_iArea,m_vOrigin.x,m_vOrigin.y,m_vOrigin.z,m_iRadius);
}

///////////
CBotTF2UpgradeBuilding :: CBotTF2UpgradeBuilding ( edict_t *pBuilding )
{
	m_pBuilding = pBuilding;
	m_fTime = 0.0f;
}

void CBotTF2UpgradeBuilding :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	pBot->wantToShoot(false);

	if (!m_fTime )
		m_fTime = engine->Time() + randomFloat(9.0f,11.0f);
	
	if ( m_fTime<engine->Time() )
	{
		complete();
	}// Fix 16/07/09
	else if ( !CBotGlobals::entityIsValid(m_pBuilding) )
	{
		fail();
		return;
	}
	else if ( !pBot->isVisible(m_pBuilding) )
	{
		if ( pBot->distanceFrom(m_pBuilding) > 200 )
			fail();
		else if ( pBot->distanceFrom(m_pBuilding) > 100 )
			pBot->setMoveTo((CBotGlobals::entityOrigin(m_pBuilding)));
		
		pBot->setLookAtTask((LOOK_EDICT));
		pBot->lookAtEdict(m_pBuilding);
	}
	else if ( CBotGlobals::entityIsValid(m_pBuilding) && CBotGlobals::entityIsAlive(m_pBuilding) )
	{
		if ( !((CBotFortress*)pBot)->upgradeBuilding(m_pBuilding) )
			fail();		
	}
	else
		fail();
}

void CBotTF2UpgradeBuilding:: debugString ( char *string )
{
	sprintf(string,"CBotTF2UpgradeBuilding");
}

void CBotHL2DMUseButton :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	static Vector vOrigin;
	
	if ( m_pButton.get() == NULL )
	{
		fail();
		return;
	}

	vOrigin = CBotGlobals::entityOrigin(m_pButton);

	if ( m_fTime == 0.0f )
	{
		m_fTime = engine->Time() + randomFloat(4.0f,6.0f);
	}

	if ( m_fTime < engine->Time() )
		complete();

	//if ( CClassInterface::getAnimCycle(m_pCharger) == 1.0f )
	//	complete();

	pBot->setLookVector(vOrigin);
	pBot->setLookAtTask(LOOK_VECTOR);

	if ( pBot->distanceFrom(m_pButton) > 96 )
	{
		pBot->setMoveTo(vOrigin);
	}
	else if ( pBot->DotProductFromOrigin(vOrigin) > 0.97f )
	{
		pBot->use();
		complete();
	}
}

void CBotHL2DMUseCharger :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	static Vector vOrigin;
	
	if ( m_pCharger.get() == NULL )
	{
		fail();
		return;
	}

	vOrigin = CBotGlobals::entityOrigin(m_pCharger);

	if ( m_fTime == 0.0f )
	{
		m_fTime = engine->Time() + randomFloat(4.0f,6.0f);
	}

	if ( m_fTime < engine->Time() )
		complete();

	if ( CClassInterface::getAnimCycle(m_pCharger) == 1.0f )
		complete();

	if ( ( m_iType == CHARGER_HEALTH ) && ( pBot->getHealthPercent() >= 0.99f ) )
		complete();
	else if ( ( m_iType == CHARGER_ARMOR ) && ( ((CHLDMBot*)pBot)->getArmorPercent() >= 0.99f ) )
		complete();

	pBot->setLookVector(vOrigin);
	pBot->setLookAtTask(LOOK_VECTOR);

	if ( pBot->distanceFrom(m_pCharger) > 96 )
	{
		pBot->setMoveTo(vOrigin);
	}
	else if ( pBot->DotProductFromOrigin(vOrigin) > 0.965925f )
	{
		pBot->use();
	}
}

void CBotGravGunPickup :: execute(CBot *pBot,CBotSchedule *pSchedule)
{
	static Vector vOrigin;
	static Vector vBotOrigin;

	if ( m_fTime == 0.0f )
	{
		m_fSecAttTime = 0;
		m_fTime = engine->Time() + randomFloat(2.0f,4.0f);
	}

	if ( m_fTime < engine->Time() )
	{
		CHLDMBot *HL2DMBot = ((CHLDMBot*)pBot);

		if (HL2DMBot->getFailedObject() && (HL2DMBot->distanceFrom(HL2DMBot->getFailedObject())<=(pBot->distanceFrom(m_Prop)+48)) )
			pBot->primaryAttack();

		HL2DMBot->setFailedObject(m_Prop);

		fail();
		return;
	}

	if ( !CBotGlobals::entityIsValid(m_Prop) || !pBot->isVisible(m_Prop) )
	{
		((CHLDMBot*)pBot)->setFailedObject(m_Prop);
		fail();
		return;
	}

	pBot->wantToChangeWeapon(false);

	vBotOrigin = pBot->getOrigin();
	vOrigin = CBotGlobals::entityOrigin(m_Prop);

	CBotWeapon *pWeapon = pBot->getCurrentWeapon();

	if ( !pWeapon || ( pWeapon->getID() != HL2DM_WEAPON_PHYSCANNON)  )
	{
		if ( !pBot->select_CWeapon(CWeapons::getWeapon(HL2DM_WEAPON_PHYSCANNON)) )
		{
			fail();
		}
	}
	else if ( pBot->distanceFrom(vOrigin) > 100 )
		pBot->setMoveTo(vOrigin);
	else if ( ((vOrigin-vBotOrigin).Length2D() < 16) && (vOrigin.z < vBotOrigin.z) )
		pBot->setMoveTo(vBotOrigin + (vBotOrigin-vOrigin)*100);
	else
		pBot->stopMoving();

	m_Weapon = INDEXENT(pWeapon->getWeaponIndex());

	pBot->setMoveLookPriority(MOVELOOK_OVERRIDE);
	pBot->setLookVector(vOrigin);
	pBot->setLookAtTask(LOOK_VECTOR);
	pBot->setMoveLookPriority(MOVELOOK_TASK);

	if ( pBot->DotProductFromOrigin(vOrigin) > 0.965925f )
	{
		edict_t *pPhys = CClassInterface::gravityGunObject(m_Weapon);

		if ( pPhys == m_Prop.get() )
			complete();
		else if ( pPhys || CClassInterface::gravityGunOpen(m_Weapon) )
		{
			if ( m_fSecAttTime < engine->Time() )
			{
				pBot->secondaryAttack();
				m_fSecAttTime = engine->Time() + randomFloat(0.25,0.75);
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////
/*
// Protect SG from Enemy
CBotTFEngiTankSentry :: CBotTFEngiTankSentry ( edict_t *pSentry, edict_t *pEnemy )
{
	m_pEnemy = pEnemy;
	m_pSentry = pSentry;
}

void CBotTFEngiTankSentry :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	
	CBotFortress *tfBot;

	if ( !pBot->isTF() )
	{
		fail();
		return;
	}
	
	tfBot = (CBotFortress*)pBot;


	if ( !CBotGlobals::entityIsAlive(m_pEnemy) )
		complete();
	else if ( !CBotGlobals::entityIsAlive(m_pSentry) || !CBotGlobals::entityIsValid(m_pSentry) || !CTeamFortress2Mod::isSentry(m_pSentry) )
		fail();
	else
	{
		Vector vOrigin;
		Vector vComp;

		vComp = CBotGlobals::entityOrigin(m_pEnemy) - CBotGlobals::entityOrigin(m_pSentry);
		vComp = vComp / vComp.Length(); // Normalise

		// find task position behind sentry
		vOrigin = CBotGlobals::entityOrigin(m_pSentry) - (vComp*80);

		if ( pBot->distanceFrom(vOrigin) > 32  )
		{
			// get into position!
			pBot->setMoveTo((vOrigin));
		}
		else
		{
			if ( !pBot->currentWeapon("tf_wrench") )
				pBot->selectWeaponName("tf_wrench");
			else
			{
				setTaskEntity();
				pBot->setLookAt(TSK_ENTITY,2);

				// Tank!!!
				pBot->duck();
				pBot->tapButton(IN_ATTACK);
			}

		}
	}
		
}
*/

////////////////////////

////////////////////////


CBotTF2WaitAmmoTask :: CBotTF2WaitAmmoTask ( Vector vOrigin )
{
	m_vOrigin = vOrigin;
	m_fWaitTime = 0.0f;
}
	
void CBotTF2WaitAmmoTask :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( !m_fWaitTime )
		m_fWaitTime = engine->Time() + randomFloat(5.0f,10.0f);

	if ( !pBot->hasSomeConditions(CONDITION_NEED_AMMO) )
	{
		complete();
	}
	else if ( m_fWaitTime < engine->Time() )
		fail();
	else if ( pBot->distanceFrom(m_vOrigin) > 100 )
	{
		pBot->setMoveTo(m_vOrigin);
	}
	else
	{
		pBot->stopMoving();
	}
}

void CBotTF2WaitAmmoTask :: debugString ( char *string )
{
	sprintf(string,"CBotTF2WaitAmmoTask");
}
///////////////////////////
CBotTaskEngiPickupBuilding :: CBotTaskEngiPickupBuilding ( edict_t *pBuilding )
{
	m_pBuilding = pBuilding;
	m_fTime = 0.0f;
}
// move building / move sentry / move disp / move tele
void CBotTaskEngiPickupBuilding :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	CBotWeapon *pWeapon = pBot->getCurrentWeapon();

	if ( m_fTime == 0.0f )
		m_fTime = engine->Time() + 6.0f;

	if ( !m_pBuilding.get() )
	{
		fail();
		return;
	}

	pBot->wantToShoot(false);
	pBot->lookAtEdict(m_pBuilding.get());
	pBot->setLookAtTask((LOOK_EDICT));

	((CBotTF2*)pBot)->updateCarrying();

	if ( ((CBotTF2*)pBot)->isCarrying() ) //if ( CBotGlobals::entityOrigin(m_pBuilding) == CBotGlobals::entityOrigin(pBot->getEdict()) )
		complete();
	else if ( m_fTime < engine->Time() )
		fail();
	else if ( !pWeapon )
		fail();
	else if ( pWeapon->getID() != TF2_WEAPON_WRENCH )
	{
		if ( !pBot->select_CWeapon(CWeapons::getWeapon(TF2_WEAPON_WRENCH)) )
			fail();
	}
	else if ( pBot->distanceFrom(m_pBuilding) < 100 )
	{
		if ( CBotGlobals::yawAngleFromEdict(pBot->getEdict(),CBotGlobals::entityOrigin(m_pBuilding)) < 25 )
		{	
			pBot->secondaryAttack();
			((CBotTF2*)pBot)->resetCarryTime();
		}
	}
	else
		pBot->setMoveTo((CBotGlobals::entityOrigin(m_pBuilding)));
}
void CBotTaskEngiPickupBuilding :: debugString ( char *string )
{
	sprintf(string,"CBotTaskEngiPickupBuilding");
}

/////////////////
CBotTaskEngiPlaceBuilding :: CBotTaskEngiPlaceBuilding ( eEngiBuild iObject, Vector vOrigin )
{
	m_vOrigin = vOrigin;
	m_fTime = 0.0f;
	m_iState = 1; // BEGIN HERE , otherwise bot will try to destroy the building
	m_iObject = iObject;
	m_iTries = 0;
}

// unused
void CBotTaskEngiPlaceBuilding :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( m_fTime == 0.0f )
	{
		m_fTime = engine->Time() + 6.0f;
		pBot->resetLookAroundTime();

		if ( CTeamFortress2Mod::buildingNearby(pBot->getTeam(),m_vOrigin) )
			m_vOrigin = m_vOrigin + Vector(randomFloat(-200.0f,200.0f),randomFloat(-200.0f,200.0f),0);
	}

	pBot->setLookVector(m_vOrigin);
	pBot->setLookAtTask((LOOK_VECTOR));

	((CBotTF2*)pBot)->updateCarrying();

	if ( !(((CBotTF2*)pBot)->isCarrying()) ) 
		complete();
	else if ( m_fTime < engine->Time() )
		fail();
	else if ( pBot->distanceFrom(m_vOrigin) < 100 )
	{		
		if ( CBotGlobals::yawAngleFromEdict(pBot->getEdict(),m_vOrigin) < 25 )
		{	
			int state = ((CBotTF2*)pBot)->engiBuildObject(&m_iState,m_iObject,&m_fTime,&m_iTries);

			if ( state == 1 )
				complete();
			else if ( state == 0 )
				fail();
			
			//pBot->primaryAttack();
		}
	}
	else
		pBot->setMoveTo(m_vOrigin);
	
	if ( pBot->hasEnemy() )
	{
		pBot->primaryAttack();
	}
}
void CBotTaskEngiPlaceBuilding :: debugString ( char *string )
{
	sprintf(string,"CBotTaskEngiPlaceBuilding");
}

/////////////////////////////
CBotBackstab :: CBotBackstab (edict_t *_pEnemy)
{
	m_fTime = 0.0f;
	pEnemy = _pEnemy;
}
	
void CBotBackstab ::execute (CBot *pBot,CBotSchedule *pSchedule)
{
	Vector vrear;
	Vector vangles;
	CBotWeapon *pBotWeapon;
	CWeapon *pWeapon;
	CBotTF2 *pTF2Bot = (CBotTF2*)pBot;

	pBot->wantToChangeWeapon(false);
	pBot->wantToShoot(false);
	pBot->wantToListen(false);

	pBotWeapon = pBot->getCurrentWeapon();

	if ( !pBotWeapon )
	{
		fail();
		pTF2Bot->waitBackstab();
		return;
	}

	pWeapon = pBotWeapon->getWeaponInfo();

	if (pWeapon == NULL)
	{
		fail();
		pTF2Bot->waitBackstab();
		return;
	}

	if ( !CBotGlobals::isAlivePlayer(pEnemy) )
		fail();

	if ( !m_fTime )
		m_fTime = engine->Time() + randomFloat(5.0f,10.0f);

	pBot->setLookAtTask(LOOK_EDICT);
	pBot->lookAtEdict(pEnemy);

	if ( m_fTime < engine->Time() )
	{
		fail();
		pTF2Bot->waitBackstab();
		return;
	}
	else if ( !pEnemy || !CBotGlobals::entityIsValid(pEnemy) || !CBotGlobals::entityIsAlive(pEnemy) )
	{
		if ( pBot->getEnemy() && (pEnemy != pBot->getEnemy()) && pBot->hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && CBotGlobals::isAlivePlayer(pBot->getEnemy()) )
		{
			pEnemy = pBot->getEnemy();
		}
		else
		{
			fail();
		}

		pTF2Bot->waitBackstab();
		return;
	}
	else if ( !pBot->isVisible(pEnemy) )
	{
		// this guy will do
		if ( pBot->getEnemy() && (pEnemy != pBot->getEnemy()) && pBot->hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && CBotGlobals::isAlivePlayer(pBot->getEnemy()) )
		{
			pEnemy = pBot->getEnemy();
		}
		else
		{
			fail();
		}

		pTF2Bot->waitBackstab();
		return;
	}
	else if (pWeapon->getID() != TF2_WEAPON_KNIFE)
	{
		if ( !pBot->select_CWeapon(CWeapons::getWeapon(TF2_WEAPON_KNIFE)) )
		{
			fail();
			pTF2Bot->waitBackstab();
			return;
		}
	}
	
	AngleVectors(CBotGlobals::entityEyeAngles(pEnemy),&vangles);
	vrear = CBotGlobals::entityOrigin(pEnemy) - (vangles * 45) + Vector(0,0,32);

	if ( pBot->distanceFrom(vrear) > 40 ) 
	{
		pBot->setMoveTo((vrear));
	}
	else
	{
		// uncloak
		if ( CTeamFortress2Mod::TF2_IsPlayerCloaked(pBot->getEdict()) )
			pTF2Bot->spyUnCloak();
		else
			pTF2Bot->handleAttack(pBotWeapon,pEnemy);
	}
}

void CBotInvestigateTask :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( m_fTime == 0 )
	{
		CWaypoint *pWaypoint = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(m_vOrigin,m_fRadius,-1));

		if ( pWaypoint == NULL )
		{
			// can't investigate
			// but other tasks depend on this, so complete it
			complete();
			return;
		}

		if ( pWaypoint->numPaths() > 0 )
		{
			for ( int i = 0; i < pWaypoint->numPaths(); i ++ )
				m_InvPoints.push_back(CWaypoints::getWaypoint(pWaypoint->getPath(i))->getOrigin());	

			m_iCurPath = randomInt(0,pWaypoint->numPaths()-1);
		}

		m_fTime = engine->Time() + m_fMaxTime;
	}

	if ( m_fTime < engine->Time() )
		complete();

	if ( m_InvPoints.size() > 0 )
	{
		Vector vPoint;

		if ( m_iState == 0 ) // goto inv point
			vPoint = m_InvPoints[m_iCurPath];
		else if ( m_iState == 1 ) // goto origin
			vPoint = m_vOrigin;

		if ( (pBot->distanceFrom(vPoint) < 80) || ((m_iState==0)&&(pBot->distanceFrom(m_vOrigin)>m_fRadius)) )
		{
			m_iState = (!m_iState) ? 1 : 0;

			if ( m_iState == 0 )
				m_iCurPath = randomInt(0,m_InvPoints.size()-1);
		}
		else
			pBot->setMoveTo(vPoint);
	}
	else
		pBot->stopMoving();
	// walk
	pBot->setMoveSpeed(CClassInterface::getMaxSpeed(pBot->getEdict())/8);
	//pBot->setLookVector();
	pBot->setLookAtTask(LOOK_AROUND);

}
///////////////////

void CBotDefendTask :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	static float fDist;

	if ( m_fTime == 0 )
	{
		if ( m_fMaxTime > 0 )
			m_fTime = engine->Time() + m_fMaxTime;
		else
			m_fTime = engine->Time() + randomFloat(20.0f,90.0f);
	}

	fDist = pBot->distanceFrom(m_vOrigin);

	if ( fDist > 200 )
		fail(); // too far -- bug
	else if ( fDist > 100 )
		pBot->setMoveTo(m_vOrigin);
	else
	{
		pBot->defending();
		
		pBot->stopMoving();

		if ( m_iWaypointType & CWaypointTypes::W_FL_CROUCH )
			pBot->duck();

		if ( m_bDefendOrigin )
		{
			pBot->setAiming(m_vDefendOrigin);
			pBot->setLookVector(m_vDefendOrigin);
		}

		pBot->setLookAtTask(m_LookTask);

		//pBot->setAiming(m_vDefendOrigin);

		/*if ( m_bDefendOrigin )
		{
			pBot->setLookAtTask(LOOK_AROUND);
			//pBot->setAiming(m_vDefendOrigin);
		}
		else
			pBot->setLookAtTask(LOOK_SNIPE);*/
	}

	if ( m_fTime < engine->Time() )
		complete();
}

//////////////////////
void CBotTF2EngiLookAfter :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	CBotFortress *tfBot = (CBotFortress*)pBot;

	if ( !m_fTime )
	{
		m_fTime = engine->Time() + randomFloat(21.0f,60.0f);
		m_fHitSentry = engine->Time() + randomFloat(1.0f,3.0f);
	}
	else if ( m_fTime < engine->Time() )
		complete();
	else if ( tfBot->lookAfterBuildings(&m_fHitSentry) )
	{
		tfBot->nextLookAfterSentryTime(engine->Time()+randomFloat(20.0f,50.0f));
		complete();
	}
}

////////////////////////
CBotTFEngiBuildTask :: CBotTFEngiBuildTask ( eEngiBuild iObject, Vector vOrigin, Vector vAiming, int iArea )
{
	m_iObject = iObject;
	m_vOrigin = vOrigin;
	m_iState = 0;
	m_fTime = 0;
	m_iTries = 0;
	m_fNextUpdateAngle = 0.0f;
	m_vAimingVector = vAiming;
	m_iArea = iArea;
}
	
void CBotTFEngiBuildTask :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	CBotFortress *tfBot;

	bool bAimingOk = true;

	//if ( !pBot->isTF() ) // shouldn't happen ever
	//	fail();

	if ( !CPoints::isValidArea(m_iArea) )
		fail();

	pBot->wantToShoot(false); // don't shoot enemies , want to build the damn thing
	pBot->wantToChangeWeapon(false); // if enemy just strike them with wrench
	pBot->wantToListen(false); // sometimes bots dont place sentries because they are looking the wrong way due to a noise

	tfBot = (CBotFortress*)pBot;

	if ( tfBot->getClass() != TF_CLASS_ENGINEER )
	{
		fail();
		return;
	}
	/*else if ( pBot->hasEnemy() && pBot->hasSomeConditions(CONDITION_SEE_CUR_ENEMY) )
	{
		fail();
		return;
	}*/

	if ( m_fTime == 0.0f )
	{
		pBot->resetLookAroundTime();

		if ( CTeamFortress2Mod::buildingNearby(pBot->getTeam(),m_vOrigin) )
			m_vOrigin = m_vOrigin + Vector(randomFloat(-200.0f,200.0f),randomFloat(-200.0f,200.0f),0);

		m_fTime = engine->Time() + randomFloat(4.0f,8.0f);

		m_fNextUpdateAngle = engine->Time() + 0.5f;
	}
	else if ( m_fTime < engine->Time() )
		fail();

	if ( m_iObject == ENGI_DISP )
	{
		edict_t *pSentry = tfBot->getSentry();

		if ( pSentry && CBotGlobals::entityIsValid(pSentry) )
		{
			Vector vSentry = CBotGlobals::entityOrigin(pSentry);
			Vector vOrigin = pBot->getOrigin();
			Vector vLookAt = vOrigin - (vSentry - vOrigin);

			m_vAimingVector = vLookAt;

			//pBot->setLookVector(vLookAt);
			//pBot->setLookAtTask(LOOK_VECTOR);
			
			//LOOK_VECTOR,11);
			//bAimingOk = pBot->DotProductFromOrigin(vLookAt) > 0.965925f; // 15 degrees // < CBotGlobals::yawAngleFromEdict(pBot->getEdict(),pBot->getLookVector()) < 15;
		}
		else
		{
			Vector vSentry = pBot->getAiming();
			Vector vOrigin = pBot->getOrigin();
			Vector vLookAt = vOrigin - (vSentry - vOrigin);

			m_vAimingVector = vLookAt;

			/*pBot->setLookVector(pBot->getAiming());
			pBot->setLookAtTask((LOOK_VECTOR));*/
			//bAimingOk = pBot->DotProductFromOrigin(vLookAt) > 0.965925f;
		}
	}

	if ( (m_iTries > 1) && (m_fNextUpdateAngle < engine->Time()) )
	{
		QAngle angles;
		Vector vforward = m_vAimingVector - pBot->getOrigin();
		vforward = vforward/vforward.Length(); // normalize

		VectorAngles(vforward,angles);

		angles.y += 22.5f; // yaw
		CBotGlobals::fixFloatAngle(&angles.y);

		AngleVectors(angles,&vforward);

		vforward = vforward/vforward.Length();

		m_vAimingVector = pBot->getOrigin()+vforward*100.0f;
		
		m_fNextUpdateAngle = engine->Time() + 0.75f;
	}

	pBot->setLookAtTask(LOOK_VECTOR);
	pBot->setLookVector(m_vAimingVector);

	bAimingOk = pBot->DotProductFromOrigin(pBot->getLookVector()) > 0.965925f; // 15 degrees

	if ( pBot->distanceFrom(m_vOrigin) > 100 )
	{
		if ( !CBotGlobals::isVisible(pBot->getEdict(),pBot->getEyePosition(),m_vOrigin) )
			fail();
		else
			pBot->setMoveTo((m_vOrigin));
	}
	else if ( bAimingOk || (m_iTries > 1) )
	{
		int state = tfBot->engiBuildObject(&m_iState,m_iObject,&m_fTime,&m_iTries);

		if ( state == 1 )
			complete();
		else if ( state == 0 )
			fail();
	}
}

void CBotTFEngiBuildTask :: debugString ( char *string )
{
	sprintf(string,"CBotTFEngiBuildTask (%d,%0.4f,%0.4f,%0.4f)",m_iObject,m_vOrigin.x,m_vOrigin.y,m_vOrigin.z);
}


///////////////////////////////////////////////////////

CFindGoodHideSpot :: CFindGoodHideSpot ( edict_t *pEntity )		
{
	m_vHideFrom = CBotGlobals::entityOrigin(pEntity);
}

CFindGoodHideSpot :: CFindGoodHideSpot ( Vector vec )
{
	m_vHideFrom = vec;
}

void CFindGoodHideSpot :: init ()
{
	// not required, should have been constructed properly
	//m_vHideFrom = Vector(0,0,0);
}

void CFindGoodHideSpot :: execute ( CBot *pBot, CBotSchedule *pSchedule )
{
	Vector vFound;

	if ( !pBot->getNavigator()->getHideSpotPosition(m_vHideFrom,&vFound) )
		fail();
	else
	{		
		pSchedule->passVector( vFound );
		complete();
	}
}

CFindPathTask :: CFindPathTask ( int iWaypointId, eLookTask looktask )
{
	m_iWaypointId = iWaypointId;
	m_LookTask = looktask;
	m_vVector = CWaypoints::getWaypoint(iWaypointId)->getOrigin();
}

void CFindPathTask :: init ()
{
	m_bNoInterruptions = false;
	m_bGetPassedVector = false;
	m_iInt = 0;
	
	//setFailInterrupt(CONDITION_SEE_CUR_ENEMY);
}

CFindPathTask :: CFindPathTask ( edict_t *pEdict )
{
	m_iWaypointId = -1;
	m_pEdict = pEdict;
	m_vVector = CBotGlobals::entityOrigin(pEdict);
	m_LookTask = LOOK_WAYPOINT;
}

void CFindPathTask :: debugString ( char *string )
{
	sprintf(string,"CFindPathTask\n m_iInt = %d\n m_vVector = (%0.4f,%0.4f,%0.4f)",m_iInt,m_vVector.x,m_vVector.y,m_vVector.z);
}

void CFindPathTask :: execute ( CBot *pBot, CBotSchedule *pSchedule )
{	
	bool bFail = false;

	if ( pSchedule->hasPassVector() )
	{
		m_vVector = pSchedule->passedVector();
		pSchedule->clearPass();
	}

	if ( (m_iInt == 0) || (m_iInt == 1) )
	{
		IBotNavigator *pNav = pBot->getNavigator();

		pBot->m_fWaypointStuckTime = 0;

#ifdef _DEBUG
		CProfileTimer *timer = CProfileTimers::getTimer(BOT_ROUTE_TIMER);

		if ( CClients::clientsDebugging(BOT_DEBUG_PROFILE) )
		{
			timer->Start();
		}
#endif

		if ( pNav->workRoute( pBot->getOrigin(),
			                   m_vVector,
							   &bFail,
							   (m_iInt==0),
							   m_bNoInterruptions, 
							   m_iWaypointId ) )
		{
			pBot->m_fWaypointStuckTime = engine->Time() + randomFloat(10.0f,15.0f);
			pBot->moveFailed(); // reset
			m_iInt = 2;
		}
		else
			m_iInt = 1;

#ifdef _DEBUG
		if ( CClients::clientsDebugging(BOT_DEBUG_PROFILE) )
		{
			timer->Stop();
		}
#endif

		pBot->debugMsg(BOT_DEBUG_NAV,"Trying to work out route");
	}

	if ( bFail )
	{
		pBot->debugMsg(BOT_DEBUG_NAV,"Route failed");
		fail();
	}
	else if ( m_iInt == 2 )
	{
		if ( pBot->m_fWaypointStuckTime == 0 )
			pBot->m_fWaypointStuckTime = engine->Time() + randomFloat(5.0f,10.0f);

		if ( m_bNoInterruptions )
		{
			pBot->debugMsg(BOT_DEBUG_NAV,"Found route");
			complete(); // ~fin~
		}

		if ( !pBot->getNavigator()->hasNextPoint() )
		{
			pBot->debugMsg(BOT_DEBUG_NAV,"Nowhere to go");
			complete(); // reached goal
		}
		else
		{			
			if ( pBot->moveFailed() )
			{
				pBot->debugMsg(BOT_DEBUG_NAV,"moveFailed() == true");
				fail();
				pBot->getNavigator()->failMove();
			}

			if ( m_pEdict )
			{
				if ( CBotGlobals::entityIsValid(m_pEdict) )
				{
					if ( pBot->isVisible(m_pEdict) )
					{
						if ( pBot->distanceFrom(m_pEdict) < pBot->distanceFrom(pBot->getNavigator()->getNextPoint()) )
							complete();
					}
				}
				else
					fail();
			}

			//// running path
			//if ( !pBot->hasEnemy() && !pBot->hasSomeConditions(CONDITION_SEE_CUR_ENEMY) )
			
			pBot->setLookAtTask(m_LookTask);
		}
	}
}

void CMoveToTask :: init () 
{ 
	fPrevDist = 0;
	//m_vVector = Vector(0,0,0);
	//m_pEdict = NULL;
}

void CMoveToTask :: debugString ( char *string )
{
	sprintf(string,"CMoveToTask\nm_vVector =(%0.4f,%0.4f,%0.4f)",m_vVector.x,m_vVector.y,m_vVector.z);	
}

CMoveToTask :: CMoveToTask ( edict_t *pEdict )
{
	m_pEdict = pEdict;
	m_vVector = CBotGlobals::entityOrigin(m_pEdict);

	//setFailInterrupt(CONDITION_SEE_CUR_ENEMY);
}

void CMoveToTask :: execute ( CBot *pBot, CBotSchedule *pSchedule )
{

	float fDistance;

	fDistance = pBot->distanceFrom(m_vVector);

	// sort out looping move to origins by using previous distance check
	if ( (fDistance < 64) || (fPrevDist&&(fPrevDist < fDistance)) )
	{
		complete();
		return;
	}
	else
	{		
		pBot->setMoveTo((m_vVector));

		if ( pBot->moveFailed() )
			fail();
	}

	fPrevDist = fDistance;
}
////////////////////////////////////////////////////



///////////////////////////////////////////////////

CBotTFRocketJump :: CBotTFRocketJump ()
{
	m_fTime = 0.0f;
	m_fJumpTime = 0.0f;
	m_iState = 0;
}

void CBotTFRocketJump :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	CBotWeapon *pBotWeapon;
	CWeapon *pWeapon;

	pBotWeapon = pBot->getCurrentWeapon();

	if ( !pBotWeapon )
	{
		fail();
		return;
	}

	pWeapon = pBotWeapon->getWeaponInfo();

	if (pWeapon == NULL)
	{
		fail();
		return;
	}

	if ( !pBot->isTF() || (((CBotFortress*)pBot)->getClass() != TF_CLASS_SOLDIER) || (pBot->getHealthPercent() < 0.3) )
	{
		fail();
	}
	else if (pWeapon->getID() != TF2_WEAPON_ROCKETLAUNCHER )
	{
		if ( !pBot->select_CWeapon(CWeapons::getWeapon(TF2_WEAPON_ROCKETLAUNCHER)) )
		{
			fail();
		}
	}
	else
	{
		CBotTF2 *tf2Bot = ((CBotTF2*)pBot);

		if ( !m_fTime )
		{
			m_fTime = engine->Time()+randomFloat(4.0f,5.0f);
		}

		if ( tf2Bot->rocketJump(&m_iState,&m_fJumpTime) == BOT_FUNC_COMPLETE )
			complete();
		else if ( m_fTime < engine->Time() )
		{
			fail();
		}
	}
}

void CBotTFRocketJump :: debugString ( char *string )
{
	sprintf(string,"CBotTFRocketJump");
}


//////////////////////////////////////////////////////

CBotTFDoubleJump :: CBotTFDoubleJump ()
{
	m_fTime = 0.0f;
}

void CBotTFDoubleJump ::execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( m_fTime == 0.0f )
	{
		pBot->tapButton(IN_JUMP);

		m_fTime = engine->Time() + 0.4;
	}
	else if ( m_fTime < engine->Time() )
	{
		pBot->jump();
		complete();
	}
}

void CBotTFDoubleJump :: debugString ( char *string )
{
	sprintf(string,"CbotTFDoublejump");
}
///////////////////////////////////////////////
void CSpyCheckAir :: execute ( CBot *pBot, CBotSchedule *pSchedule )
{
	CBotTF2 *pBotTF2 = (CBotTF2*)pBot;
	CBotWeapon *pWeapon;
	CBotWeapon *pChooseWeapon;
	CBotWeapons *pWeaponList;
	static int iAttackProb;

	if ( m_fTime == 0.0f )
	{
		// record the number of people I see now
		int i;
		edict_t *pPlayer;
/*		edict_t *pDisguised;

		int iClass;
		int iTeam;
		int iIndex;
		int iHealth;
*/
		seenlist = 0;		

		for ( i = 1; i <= gpGlobals->maxClients; i ++ )
		{
			pPlayer = INDEXENT(i);

			if ( pPlayer == pBot->getEdict() )
				continue;

			if ( !CBotGlobals::entityIsValid(pPlayer) )
				continue;

			//if ( CClassInterface::getTeam(pPlayer) != pBot->getTeam() )
			//{
				if ( CClassInterface::getTF2Class(pPlayer) == TF_CLASS_SPY )
				{
					//CClassInterface::getTF2SpyDisguised(pPlayer,&iClass,&iTeam,&iIndex,&iHealth);

					//if ( (iIndex > 0) && (iIndex <= gpGlobals->maxClients) )

					if ( CTeamFortress2Mod::TF2_IsPlayerCloaked(pPlayer) )
						continue; // can't see
				}
			//}

			if ( pBot->isVisible(pPlayer) )
				seenlist |= (1<<(i-1));
		}
		
		m_fTime = engine->Time() + randomFloat(2.0f,5.0f);
		m_fNextCheckUnseen = engine->Time() + 0.1f;
	}

	if ( m_fTime < engine->Time() )
		complete();

	if ( pBot->hasEnemy() )
		complete();

	if ( (m_pUnseenBefore==NULL) && (m_fNextCheckUnseen < engine->Time()) )
	{
		int i;
		edict_t *pPlayer;

		seenlist = 0;

		for ( i = 1; i <= gpGlobals->maxClients; i ++ )
		{
			pPlayer = INDEXENT(i);

			if ( pPlayer == pBot->getEdict() )
				continue;

			if ( !CBotGlobals::entityIsValid(pPlayer) )
				continue;

			if ( CClassInterface::getTF2Class(pPlayer) == TF_CLASS_SPY )
			{
					//CClassInterface::getTF2SpyDisguised(pPlayer,&iClass,&iTeam,&iIndex,&iHealth);

					//if ( (iIndex > 0) && (iIndex <= gpGlobals->maxClients) )

				if ( CTeamFortress2Mod::TF2_IsPlayerCloaked(pPlayer) )
					continue; // can't see but may still be in visible list
			}

			if ( pBot->isVisible(pPlayer) )
			{
				if ( !(seenlist & (1<<(i-1))) )
				{
					m_pUnseenBefore = pPlayer;
					seenlist |= (1<<(i-1)); //add to list
					break;
				}
			}
			
		}

		if ( m_pUnseenBefore != NULL )
		{
			// add more time
			m_fTime = engine->Time() + randomFloat(2.0f,5.0f);
		}

		m_fNextCheckUnseen = engine->Time() + 0.1f;
	}
	else if ( m_pUnseenBefore && (!CBotGlobals::entityIsAlive(m_pUnseenBefore) || !CBotGlobals::entityIsValid(m_pUnseenBefore) || !pBot->isVisible(m_pUnseenBefore) ) ) 
	{
		m_pUnseenBefore = NULL;
		m_fNextCheckUnseen = 0.0f;
	}

	if ( m_pUnseenBefore == NULL )
	{
		// automatically look at danger points
		pBot->updateDanger(50.0f);

		pBot->setLookAtTask(LOOK_WAYPOINT);
	}
	else
	{
		// smack him
		pBot->lookAtEdict(m_pUnseenBefore);
		pBot->setLookAtTask(LOOK_EDICT);
		pBot->setMoveTo(CBotGlobals::entityOrigin(m_pUnseenBefore));
	}
/*
	TF_CLASS_CIVILIAN = 0,
	TF_CLASS_SCOUT,
	TF_CLASS_SNIPER,
	TF_CLASS_SOLDIER,
	TF_CLASS_DEMOMAN,
	TF_CLASS_MEDIC,
	TF_CLASS_HWGUY,
	TF_CLASS_PYRO,
	TF_CLASS_SPY,
	TF_CLASS_ENGINEER,
	TF_CLASS_MAX*/

	pWeapon = pBot->getCurrentWeapon();

	switch ( pBotTF2->getClass() )
	{
	case TF_CLASS_PYRO:

		pWeaponList = pBot->getWeapons();

		pChooseWeapon = pWeaponList->getWeapon(CWeapons::getWeapon(TF2_WEAPON_FLAMETHROWER));
		
		if ( !pChooseWeapon->outOfAmmo(pBot) )
		{
			// use flamethrower
			iAttackProb = 90;
			break;
		}
		// move down to melee if out of ammo
	default:
		iAttackProb = 75;
		pChooseWeapon = pBot->getBestWeapon(NULL,true,true,true);
		break;
	}

	if ( m_pUnseenBefore )
	{
		pBot->setMoveTo(CBotGlobals::entityOrigin(m_pUnseenBefore));
	}

	if ( pChooseWeapon && (pWeapon != pChooseWeapon) )
	{
		if ( !pBot->selectBotWeapon(pChooseWeapon) )
		{
			fail();
		}
	}
	else
	{
		if ( randomInt(0,100) < iAttackProb )
			pBot->primaryAttack();
	}
	
}

void CSpyCheckAir :: debugString (char *string)
{
	sprintf(string,"CSpyCheckAir: checking for spies");
}

/////////////////////////////////////////////
CBotRemoveSapper :: CBotRemoveSapper ( edict_t *pBuilding, eEngiBuild id )
{
	m_fTime = 0.0f;
	m_pBuilding = MyEHandle(pBuilding);
	m_id = id;
	m_fHealTime = 0.0f;
}
	
void CBotRemoveSapper :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	//int i = 0;
	edict_t *pBuilding;
	CBotTF2 *pTF2Bot = (CBotTF2*)pBot;

	pBot->wantToShoot(false);
	pBot->wantToChangeWeapon(false);

	if ( m_fTime == 0.0f )
		m_fTime = engine->Time() + randomFloat(8.0f,12.0f);

	pBuilding = m_pBuilding.get();

	if ( !pBuilding )
	{
		fail();
		return;
	}

	if ( m_id == ENGI_DISP )
	{
		if ( !CTeamFortress2Mod::isDispenserSapped(pBuilding) )
		{
			if ( m_fHealTime == 0.0f )
				m_fHealTime = engine->Time() + randomFloat(2.0f,3.0f);
			else if ( m_fHealTime < engine->Time() )
			{
				complete();
				return;
			}
		}
		else if ( m_fHealTime > 0.0f )
		{
			fail();
			pTF2Bot->waitRemoveSap();
			return;
		}
	}
	else if ( m_id == ENGI_TELE )
	{
		if ( !CTeamFortress2Mod::isTeleporterSapped(pBuilding) )
		{
			if ( m_fHealTime == 0.0f )
				m_fHealTime = engine->Time() + randomFloat(2.0f,3.0f);
			else if ( m_fHealTime < engine->Time() )
			{
				complete();
				return;
			}
		}
		else if ( m_fHealTime > 0.0f )
		{
			fail();
			pTF2Bot->waitRemoveSap();
			return;
		}
	}
	else if ( m_id == ENGI_SENTRY )
	{
		if ( !CTeamFortress2Mod::isSentrySapped(pBuilding) )
		{
			if ( m_fHealTime == 0.0f )
				m_fHealTime = engine->Time() + randomFloat(2.0f,3.0f);
			else if ( m_fHealTime < engine->Time() )
			{
				complete();
				return;
			}
		}
		else if ( m_fHealTime > 0.0f )
		{
			fail();
			pTF2Bot->waitRemoveSap();
			return;
		}
	}
	
	if ( m_fTime<engine->Time() )
	{
		fail();
	}// Fix 16/07/09
	else if ( !pBot->isVisible(pBuilding) )
	{
		if ( pBot->distanceFrom(pBuilding) > 200 )
			fail();
		else if ( pBot->distanceFrom(pBuilding) > 100 )
			pBot->setMoveTo((CBotGlobals::entityOrigin(pBuilding)));
		
		pBot->setLookAtTask((LOOK_EDICT));
		pBot->lookAtEdict(pBuilding);
	}
	else
	{
		if ( !((CBotFortress*)pBot)->upgradeBuilding(pBuilding,true) )
			fail();
	}
}

////////////////////////////////////////////////////

CBotTF2Snipe :: CBotTF2Snipe ( Vector vOrigin, float fYaw, int iArea )
{
	QAngle angle;
	m_fTime = 0.0f;
	angle = QAngle(0,fYaw,0);
	AngleVectors(angle,&m_vAim);
	m_vAim = vOrigin + (m_vAim*1024);
	m_vOrigin = vOrigin;
	m_fEnemyZ = m_vAim.z;
	m_iArea = iArea;
}
	
void CBotTF2Snipe :: execute (CBot *pBot,CBotSchedule *pSchedule)
{

	CBotWeapon *pBotWeapon;
	CWeapon *pWeapon;

	if ( !CPoints::isValidArea(m_iArea) )
		fail(); // move up

	pBot->wantToShoot(false);
	pBot->wantToListen(false);

	if ( m_fTime == 0.0f )
	{
		m_fTime = engine->Time() + randomFloat(40.0f,90.0f);
		pBot->secondaryAttack();
		pBot->resetLookAroundTime();
	}

	pBotWeapon = pBot->getCurrentWeapon();

	if ( !pBotWeapon )
	{
		fail();
		return;
	}

	pWeapon = pBotWeapon->getWeaponInfo();

	if (pWeapon == NULL)
	{
		fail();
		return;
	}

	if ( !pBot->isTF() || (((CBotFortress*)pBot)->getClass() != TF_CLASS_SNIPER) || (pBot->getHealthPercent() < 0.2) )
	{
		if ( CTeamFortress2Mod::TF2_IsPlayerZoomed(pBot->getEdict()) )
			pBot->secondaryAttack();

		fail();
	}
	else if (pWeapon->getID() != TF2_WEAPON_SNIPERRIFLE )
	{
		if ( !pBot->select_CWeapon(CWeapons::getWeapon(TF2_WEAPON_SNIPERRIFLE)) )
		{
			fail();
		}
	}
	else if ( pBotWeapon->getAmmo(pBot) < 1 )
	{
		if ( CTeamFortress2Mod::TF2_IsPlayerZoomed(pBot->getEdict()) )
			pBot->secondaryAttack();

		complete();
	}
	else if ( pBot->distanceFrom(m_vOrigin) > 200 )
	{
		if ( CTeamFortress2Mod::TF2_IsPlayerZoomed(pBot->getEdict()) )
			pBot->secondaryAttack();
		// too far away
		fail();
	}
	else if ( pBot->distanceFrom(m_vOrigin) > 100 )
	{
		pBot->setMoveTo(m_vOrigin);
	}
	else
	{
		pBot->stopMoving();

		if ( pBot->hasEnemy() )
		{
			// careful that the round may have not started yet
			if ( CTeamFortress2Mod::hasRoundStarted() )
			{
				pBot->setMoveLookPriority(MOVELOOK_ATTACK);

				pBot->setLookAtTask(LOOK_ENEMY);

				pBot->handleAttack(pBotWeapon,pBot->getEnemy());

				pBot->setMoveLookPriority(MOVELOOK_TASK);
			}

			m_fEnemyZ = CBotGlobals::entityOrigin(pBot->getEnemy()).z;

		}
		else
		{
			pBot->setLookAtTask(LOOK_SNIPE);
			pBot->setAiming(Vector(m_vAim.x,m_vAim.y,m_fEnemyZ));
//			pBot->setAiming(m_vAiming);

			if (m_fTime<engine->Time() )
			{
				if ( CTeamFortress2Mod::TF2_IsPlayerZoomed(pBot->getEdict()) )
					pBot->secondaryAttack();

				complete();
			}
			else
			{
				if ( !CTeamFortress2Mod::TF2_IsPlayerZoomed(pBot->getEdict()) )
					pBot->secondaryAttack();
			}
		}
	}
}

/////////////////////////////////////////////////////

CBotTF2SpySap :: CBotTF2SpySap ( edict_t *pBuilding, eEngiBuild id )
{
	m_pBuilding = MyEHandle(pBuilding);
	m_fTime = 0.0f;
	m_id = id;
}

void CBotTF2SpySap :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	edict_t *pBuilding;

	if ( !pBot->isTF() )
	{
		fail();
		return;
	}

	if ( !m_fTime )
		m_fTime = engine->Time() + randomFloat(4.0f,6.0f);

	CBotTF2 *tf2Bot = (CBotTF2*)pBot;
	CBotWeapon *weapon;
	pBot->wantToShoot(false);

	pBuilding = m_pBuilding.get();

	if ( !pBuilding )
	{
		complete();
		return;
	}

	if ( m_id == ENGI_SENTRY )
	{
		if ( CTeamFortress2Mod::isSentrySapped(pBuilding) )
		{
			complete();
			return;
		}
	}
	else if ( m_id == ENGI_DISP )
	{
		if ( CTeamFortress2Mod::isDispenserSapped(pBuilding) )
		{
			complete();
			return;
		}
	}
	else if ( m_id == ENGI_TELE) 
	{
		if ( CTeamFortress2Mod::isTeleporterSapped(pBuilding) )
		{
			complete();
			return;
		}
	}

	pBot->lookAtEdict(pBuilding);
	pBot->setLookAtTask((LOOK_EDICT));
	weapon = tf2Bot->getCurrentWeapon();

	// time out
	if ( m_fTime < engine->Time() )
		fail();
	else if ( weapon->getID() != TF2_WEAPON_BUILDER )
	{
		helpers->ClientCommand(pBot->getEdict(),"build 3 0");
	}
	else 
	{
		if ( pBot->distanceFrom(pBuilding) > 100 )
		{
			pBot->setMoveTo((CBotGlobals::entityOrigin(pBuilding)));
		}
		else
		{
			if ( CTeamFortress2Mod::TF2_IsPlayerCloaked(pBot->getEdict()) )
			{
				pBot->secondaryAttack();
				tf2Bot->waitCloak();
			}
			else if ( randomInt(0,1) )
				pBot->tapButton(IN_ATTACK);
			//complete();
		}
	}

}

void CBotTF2SpySap :: debugString ( char *string )
{
	sprintf(string,"CBotTF2SpySap");
}


/////////////////////////////////////////////////////
CBotTFUseTeleporter :: CBotTFUseTeleporter ( edict_t *pTele )
{// going to use this 
	
	m_pTele = pTele;
	m_fTime = 0.0;
}

void CBotTFUseTeleporter :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( !m_pTele || !CBotGlobals::entityIsValid(m_pTele) )
	{
		fail();
		return;
	}

	if ( !pBot->isTF() )
	{
		if ( ((CBotFortress*)pBot)->hasFlag() )
		{
			fail();
			return;
		}
	}

	if ( !m_fTime )
	{
		// initialize
		m_fTime = engine->Time() + 13.0f;
		m_vLastOrigin = pBot->getOrigin();
	}

	// FIX BUG
	//if ( !((CBotFortress*)pBot)->isTeleporterUseful(m_pTele) )
	//	fail();

	if ( m_fTime < engine->Time() )
	{
		if ( CClients::clientsDebugging(BOT_DEBUG_TASK) )
			CClients::clientDebugMsg(BOT_DEBUG_TASK,"TELEPORT: TIMEOUT",pBot);

		fail();

	}
	else
	{
		if ( CTeamFortress2Mod::getTeleporterExit(m_pTele) ) // exit is still alive?
		{
			Vector vTele = CBotGlobals::entityOrigin(m_pTele);		

			if ( pBot->distanceFrom(vTele) > 48 )
			{
				pBot->setMoveTo((vTele));		
				
				if ( (m_vLastOrigin - pBot->getOrigin()).Length() > 50 )
				{
					pBot->getNavigator()->freeMapMemory(); // restart navigator
				
					complete(); // finished
				}
			}
			else
			{
				pBot->stopMoving();
			}
		
			m_vLastOrigin = pBot->getOrigin();

		}
		else
			fail();
	}
}

void CBotTFUseTeleporter :: debugString ( char *string )
{
	sprintf(string,"CBotTFUseTeleporter\nm_pTele = %x",(int)m_pTele.get());
}

///////////////////////////////////////////////////

CAttackEntityTask :: CAttackEntityTask ( edict_t *pEdict )
{
	m_pEdict = pEdict;
}

void CAttackEntityTask :: debugString ( char *string )
{
	int id = -1;

	if ( m_pEdict )
      id = ENTINDEX(m_pEdict);

	sprintf(string,"CAttackEntityTask (%d)",id);	
}

void CAttackEntityTask :: init ()
{
	//setFailInterrupt ( CONDITION_ENEMY_OBSCURED );
	//setCompleteInterrupt ( CONDITION_ENEMY_DEAD );
}

void CAttackEntityTask :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	CBotWeapon *pWeapon;

	if ( m_pEdict.get() == NULL )
	{
		fail();
		return;
	}

	if ( !pBot->isEnemy(m_pEdict) )
	{
		complete();
		return;
	}

	if ( !pBot->isVisible(m_pEdict) )
	{
		fail();
		return;
	}

	if ( pBot->hasSomeConditions(CONDITION_ENEMY_DEAD) )
	{
		fail();
		return;
	}

	pWeapon = pBot->getBestWeapon(m_pEdict);

	if ( (pWeapon != NULL) && (pWeapon != pBot->getCurrentWeapon()) && pWeapon->getWeaponIndex() )
	{
		//pBot->selectWeaponSlot(pWeapon->getWeaponInfo()->getSlot());
		pBot->selectWeapon(pWeapon->getWeaponIndex());
	}

	pBot->setEnemy(m_pEdict);

	pBot->setLookAtTask((LOOK_ENEMY));

	if ( !pBot->handleAttack ( pWeapon, m_pEdict ) )
		fail();
}

///
CThrowGrenadeTask :: CThrowGrenadeTask (CBotWeapon *pWeapon, int ammo, Vector vLoc )
{
	m_pWeapon = pWeapon;
	m_fTime = 0;
	m_vLoc = vLoc;

	m_fHoldAttackTime = 0;
	m_iAmmo = ammo;
}

void CThrowGrenadeTask ::init()
{
	m_fTime = 0;
}

void CThrowGrenadeTask::debugString(char *string)
{
	sprintf(string,"CThrowGrenadeTask\nm_vLoc =(%0.4f,%0.4f,%0.4f)\nfTime = %0.4f",m_vLoc.x,m_vLoc.y,m_vLoc.z,m_fTime);	
}
void CThrowGrenadeTask ::execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( m_fTime == 0 )
	{
		m_fTime = engine->Time() + 2.5f;

		if ( sv_gravity )
		{
			float fFraction = pBot->distanceFrom(m_vLoc)/MAX_GREN_THROW_DIST;

			m_vLoc.z = m_vLoc.z + (sv_gravity->GetFloat() * randomFloat(1.5f,2.5f) * fFraction);
		}
	}

	if ( m_fTime < engine->Time() )
		fail();

	if ( m_pWeapon->getAmmo(pBot) < m_iAmmo )
	{
		pBot->grenadeThrown();
		complete();
		return;
	}

	if ( !m_pWeapon )
	{
		fail();
		return;
	}

	CBotWeapon *pWeapon;

	pWeapon = pBot->getCurrentWeapon();
	pBot->wantToChangeWeapon(false);

	if ( pWeapon && pWeapon->isGravGun() && CClassInterface::gravityGunObject(INDEXENT(pWeapon->getWeaponIndex())) )
	{
		// drop it
		if ( randomInt(0,1) )
			pBot->primaryAttack();	
		else
			pBot->secondaryAttack();
	}
	else if ( pWeapon != m_pWeapon )
	{
		pBot->selectBotWeapon(m_pWeapon);
	}
	else
	{
		pBot->setLookVector(m_vLoc);
		pBot->setLookAtTask(LOOK_VECTOR);

		if ( pBot->DotProductFromOrigin(m_vLoc) > 0.98 )
		{
			if ( randomInt(0,1) )
				pBot->primaryAttack();
		}
		else if ( pBot->hasSomeConditions(CONDITION_SEE_CUR_ENEMY) )
		{
			pBot->primaryAttack();
			fail();
		}
	}
}
///

void CAutoBuy :: init () 
{ 
	m_bTimeset = false;
}

void CAutoBuy :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( !m_bTimeset )
	{
		m_bTimeset = true;
		m_fTime = engine->Time() + randomFloat(2.0,4.0);
	}
	else if ( m_fTime < engine->Time() )
	{
		engine->SetFakeClientConVarValue(pBot->getEdict(),"cl_autobuy","m4a1 ak47 famas galil p90 mp5 primammo secammo defuser vesthelm vest");
		//helpers->ClientCommand(pBot->getEdict(),"setinfo cl_autobuy \"m4a1 ak47 famas galil p90 mp5 primammo secammo defuser vesthelm vest\"\n");
		helpers->ClientCommand(pBot->getEdict(),"autobuy\n");	
		complete();
	}
}

CFindLastEnemy::CFindLastEnemy (Vector vLast,Vector vVelocity)
{
	setCompleteInterrupt(CONDITION_SEE_CUR_ENEMY);
	m_vLast = vLast+(vVelocity*10);
	m_fTime = 0;
}

void CFindLastEnemy::execute ( CBot *pBot, CBotSchedule *pSchedule )
{
	if ( m_fTime == 0 )
		m_fTime = engine->Time() + randomFloat(2.0,4.0);

	if ( !pBot->moveToIsValid() || pBot->moveFailed() )
		fail();
	if ( pBot->distanceFrom(m_vLast) > 80 )
		pBot->setMoveTo(m_vLast);
	else
		pBot->stopMoving();

	pBot->setLookAtTask(LOOK_AROUND);

	if ( m_fTime < engine->Time() )
		complete();
}
////////////////////////////////////////////
CCrouchHideTask :: CCrouchHideTask( edict_t *pHideFrom )
{
	m_pHideFrom = pHideFrom;
	m_vLastSeeVector = CBotGlobals::entityOrigin(pHideFrom);
	m_bCrouching = true; // duck
	m_fChangeTime = 0.0f;
	m_fHideTime = 0.0f;
}

void CCrouchHideTask :: init ()
{
	m_bCrouching = true; // duck
	m_fChangeTime = 0.0f;
	m_fHideTime = 0.0f;
}

void CCrouchHideTask :: debugString ( char *string )
{
	sprintf(string,"CHideTask\nm_pHideFrom =(%x)",(int)m_pHideFrom.get());
}

void CCrouchHideTask :: execute ( CBot *pBot, CBotSchedule *pSchedule )
{
	if ( m_pHideFrom.get() == NULL )
	{
		complete();
		return;
	}
	if ( CBotGlobals::entityIsAlive(m_pHideFrom) )
	{
		complete();
		return;
	}

	if ( m_fHideTime == 0 )
		m_fHideTime = engine->Time() + randomFloat(7.0f,14.0f);

	if ( m_fChangeTime == 0.0f )
		m_fChangeTime = engine->Time() + randomFloat(1.0f,3.0f);

	pBot->stopMoving();	

	if ( pBot->isVisible(m_pHideFrom) )
		m_vLastSeeVector = CBotGlobals::entityOrigin(m_pHideFrom);

	pBot->setLookVector(m_vLastSeeVector);

	pBot->setLookAtTask(LOOK_VECTOR);

	if ( m_fChangeTime < engine->Time() )
	{
		m_bCrouching = !m_bCrouching;
		m_fChangeTime = engine->Time() + randomFloat(1.0f,3.0f);
	}

	if ( m_bCrouching )
		pBot->duck(true);

	if ( m_fHideTime < engine->Time() )
		complete();

}
////////////////////////////////////////////////////////
CHideTask :: CHideTask( Vector vHideFrom )
{
	m_vHideFrom = vHideFrom;
}


void CHideTask :: debugString ( char *string )
{
	sprintf(string,"CHideTask\nm_vHideFrom =(%0.4f,%0.4f,%0.4f)",m_vHideFrom.x,m_vHideFrom.y,m_vHideFrom.z);	
}

void CHideTask :: init ()
{
	m_fHideTime = 0;	
}

void CHideTask :: execute ( CBot *pBot, CBotSchedule *pSchedule )
{
	pBot->stopMoving();	
	pBot->setLookVector(m_vHideFrom);
	pBot->setLookAtTask(LOOK_VECTOR);
	pBot->duck(true);

	if ( m_fHideTime == 0 )
		m_fHideTime = engine->Time() + randomFloat(5.0,10.0);

	if ( m_fHideTime < engine->Time() )
		complete();
}
///////////////////////////////////////////
CBotTF2DemomanPipeJump :: CBotTF2DemomanPipeJump ( CBot *pBot, Vector vWaypointGround, 
						Vector vWaypointNext, 
						CBotWeapon *pWeapon )
{
	m_iStartingAmmo = pWeapon->getClip1(pBot);
	m_vStart = vWaypointGround - Vector(0,0,48.0);
	m_vEnd = vWaypointNext;
	m_pPipeBomb = NULL;
	m_fTime = 0;
	m_iState = 0;
	m_pWeapon = pWeapon;
	m_bFired = false;
}

void CBotTF2DemomanPipeJump :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( m_fTime == 0 )
	{
		// init
		m_fTime = engine->Time() + randomFloat(5.0f,10.0f);
	}

	if ( m_fTime < engine->Time() )
		fail(); // time out

	if ( pBot->getCurrentWeapon() != m_pWeapon )
	{
		pBot->selectBotWeapon(m_pWeapon);
		return;
	}

	if ( m_pPipeBomb )
	{
		if ( !CBotGlobals::entityIsValid(m_pPipeBomb) )
		{
			fail();
			return;
		}
	}

	switch ( m_iState )
	{
	case 0:
		if ( m_pWeapon->getClip1(pBot) == 0 )
		{
			if ( m_pWeapon->getAmmo(pBot) == 0 )
				fail();
			else if ( randomInt(0,1) )
				pBot->tapButton(IN_RELOAD);
		}
		else
		{
			if ( !m_bFired && !m_iStartingAmmo )
				m_iStartingAmmo = m_pWeapon->getClip1(pBot);
			else if ( m_bFired && (m_iStartingAmmo > m_pWeapon->getClip1(pBot)) )
			{
				// find pipe bomb
				m_pPipeBomb = CClassInterface::FindEntityByClassnameNearest(pBot->getOrigin(),"tf_projectile_pipe_remote",150.0f,NULL);

				if ( m_pPipeBomb )
				{
					// set this up incase of fail, the bot knows he has a sticky there
					((CBotTF2*)pBot)->setStickyTrapType(m_vStart,TF_TRAP_TYPE_ENEMY);
					m_iState++;
				}
				else
					fail();
			}
			else
			{
				pBot->setLookVector(m_vStart);
				pBot->setLookAtTask(LOOK_VECTOR);

				if ( pBot->distanceFrom(m_vStart) < 150 )
				{
					if ( pBot->DotProductFromOrigin(m_vStart) > 0.99 )
					{
						if ( randomInt(0,1) )
						{
							pBot->primaryAttack();
							m_bFired = true;
						}
					}
				}
				else
				{
					pBot->setMoveTo(m_vStart);
				}
			}
		}
		break;
	case 1:
		{
			Vector v_comp;
			Vector v_startrunup;
			Vector v_pipe;
			Vector vel;
			extern ConVar rcbot_demo_runup_dist;

			if ( CClassInterface::getVelocity(m_pPipeBomb,&vel) )
			{
				if ( vel.Length() > 1.0 )
					break; // wait until the pipe bomb has rested
			}

			v_comp = m_vEnd - m_vStart;
			v_comp = v_comp / v_comp.Length();

			v_pipe =  CBotGlobals::entityOrigin(m_pPipeBomb);
			v_startrunup = v_pipe - (v_comp * rcbot_demo_runup_dist.GetFloat());
			v_startrunup.z = v_pipe.z;

			pBot->lookAtEdict(m_pPipeBomb);
			pBot->setLookAtTask(LOOK_EDICT);

			// m_pPipeBomb != NULL

			// run up and jump time

			if ( pBot->distanceFrom(v_startrunup) < 52.0f )
				m_iState++;
			
			pBot->setMoveTo(v_startrunup);
		}
		break;
	case 2:
		{
			Vector v_comp;
			extern ConVar rcbot_demo_runup_dist;
			Vector v_endrunup;
			Vector v_pipe;

			v_comp = m_vEnd - m_vStart;
			v_comp = v_comp / v_comp.Length();
			v_pipe = CBotGlobals::entityOrigin(m_pPipeBomb);

			v_endrunup = v_pipe + (v_comp * rcbot_demo_runup_dist.GetFloat());
			v_endrunup.z = v_pipe.z;

			pBot->setLookVector(m_vEnd);
			pBot->setLookAtTask(LOOK_VECTOR);

			// m_pPipeBomb != NULL

			// run up and jump time

			if ( pBot->distanceFrom(v_endrunup) < 48.0f )
			{
				m_iState++;
			}
			
			pBot->setMoveTo(v_endrunup);
		}
		break;
	case 3:
		pBot->jump();
		m_iState++;
		break;
	case 4:
		{
			Vector vel;
		
			if ( CClassInterface::getVelocity(pBot->getEdict(),&vel) )
			{
				if ( vel.z > 10 )
				{
					((CBotTF2*)pBot)->detonateStickies(true);
					complete();
				}
			}
			else
			{
				((CBotTF2*)pBot)->detonateStickies(true);
				complete();
			}
		}
		break;
	default:
		fail();
	}
}

//////////////////////////////////////////
CBotTF2DemomanPipeEnemy :: CBotTF2DemomanPipeEnemy ( Vector vStand, Vector vBlastPoint, CBotWeapon *pPipeLauncher, Vector vEnemy, edict_t *pEnemy )
{
	m_vEnemy = vEnemy;
	m_pEnemy = MyEHandle(pEnemy);
	m_fTime = 0.0f;
	m_vAim = vBlastPoint;
	m_pPipeLauncher = pPipeLauncher;
	m_vStand = vStand;
	m_fHoldAttackTime = 0.0f;
	m_fHeldAttackTime = 0.0f;
}

void CBotTF2DemomanPipeEnemy :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( pBot->distanceFrom(m_vStand) > 200 )
		fail();

	if ( m_fTime == 0 )
	{
		m_vAim = (m_vEnemy - pBot->getOrigin())/2;

		if ( sv_gravity )
		{
			float fFraction = pBot->distanceFrom(m_vEnemy)/TF2_GRENADESPEED;

			m_vAim.z = m_vEnemy.z + (sv_gravity->GetFloat() * randomFloat(0.9f,1.1f) * fFraction);

			//fFraction = sv_gravity->GetFloat() / TF2_GRENADESPEED;
		}

		m_fHoldAttackTime = (pBot->distanceFrom(m_vEnemy)/512.0f) - 1.0f;

		if ( m_fHoldAttackTime < 0.0f )
			m_fHoldAttackTime = 0.0f;

		m_vAim = m_vStand + m_vAim;

		/*
		if ( sv_gravity )
		{
			float fFraction = ((m_vAim-m_vStand).Length())/MAX_GREN_THROW_DIST;

			m_vAim.z = m_vAim.z + (sv_gravity->GetFloat() * randomFloat(1.5f,2.5f) * fFraction);
		}*/

		m_fTime = engine->Time() + randomFloat(5.0f,10.0f);
	}

	if ( !CBotGlobals::entityIsValid(m_pEnemy) || !CBotGlobals::entityIsAlive(m_pEnemy) || (m_fTime < engine->Time()) )
	{
		// blow up any grens before we finish
		//if ( m_pEnemy.get() && pBot->isVisible(m_pEnemy.get()) )
		((CBotTF2*)pBot)->detonateStickies();

		complete();
	}

	pBot->wantToChangeWeapon(false);
	pBot->wantToShoot(false);

	if ( (m_pPipeLauncher->getAmmo(pBot) + m_pPipeLauncher->getClip1(pBot)) == 0 )
	{
		if ( pBot->isVisible(m_pEnemy.get()) )
			((CBotTF2*)pBot)->detonateStickies();

		complete();
	}
	else if ( pBot->getCurrentWeapon() != m_pPipeLauncher )
		pBot->selectBotWeapon(m_pPipeLauncher);
	else if ( m_pPipeLauncher->getClip1(pBot) == 0 )
	{
		if ( randomInt(0,1) )
			pBot->reload();
	}
	else if ( pBot->distanceFrom(m_vStand) > 100.0f )
		pBot->setMoveTo(m_vStand);
	else
	{
		pBot->setLookAtTask(LOOK_VECTOR);
		pBot->setLookVector(m_vAim);
		pBot->stopMoving();

		if ( pBot->DotProductFromOrigin(m_vAim) > 0.99 )
		{
			float fTime = engine->Time();

			if ( m_fHeldAttackTime == 0 )
				m_fHeldAttackTime = fTime + m_fHoldAttackTime + randomFloat(0.0,0.15);

			if ( m_fHeldAttackTime > fTime)
				pBot->primaryAttack(true);
			else
			{
				if ( m_fHeldAttackTime < (fTime - 0.1f) )
					m_fHeldAttackTime = 0;

				pBot->letGoOfButton(IN_ATTACK);
			}

			((CBotTF2*)pBot)->setStickyTrapType(m_vEnemy,TF_TRAP_TYPE_ENEMY);
		}
	}
}


//////////////////////////////////////////
CBotTF2DemomanPipeTrap :: CBotTF2DemomanPipeTrap ( eDemoTrapType type, Vector vStand, Vector vLoc, Vector vSpread, bool bAutoDetonate)
{
	m_vPoint = vLoc;
	m_vLocation = vLoc;
	m_vSpread = vSpread;
	m_iState = 0;
	m_iStickies = 6;
	m_iTrapType = type;
	m_vStand = vStand;
	m_fTime = 0.0f;
	m_bAutoDetonate = bAutoDetonate;
}
	
void CBotTF2DemomanPipeTrap :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	bool bFail = false;
	CBotTF2 *pTF2Bot = (CBotTF2*)pBot;

	pBot->wantToChangeWeapon(false);

	if ( pBot->getEnemy() && pBot->hasSomeConditions(CONDITION_SEE_CUR_ENEMY) )
	{
		if ( CTeamFortress2Mod::hasRoundStarted() )
		{
			pBot->secondaryAttack();
			fail();
		}
	}
	
	if ( pTF2Bot->deployStickies(m_iTrapType,m_vStand,m_vLocation,m_vSpread,&m_vPoint,&m_iState,&m_iStickies,&bFail,&m_fTime) )
	{
		complete();

		if ( m_bAutoDetonate )
			pBot->secondaryAttack();
	}

	if ( bFail )
		fail();
}
/////////

CMessAround::CMessAround ( edict_t *pFriendly, int iMaxVoiceCmd )
{
	m_fTime = 0.0f;
	m_pFriendly = pFriendly;
	m_iType = randomInt(0,3);
	m_iMaxVoiceCmd = iMaxVoiceCmd;
}

void CMessAround::execute ( CBot *pBot, CBotSchedule *pSchedule )
{
	if ( !m_pFriendly || !CBotGlobals::entityIsValid(m_pFriendly) )
		fail();

	// smack the friendly player with my melee attack
	switch ( m_iType )
	{
	case 0:
		{
		Vector origin = CBotGlobals::entityOrigin(m_pFriendly);

		pBot->setLookVector(origin);
		pBot->setLookAtTask((LOOK_VECTOR));


		if ( pBot->distanceFrom(m_pFriendly) > 100 )
		{
			pBot->setMoveTo((origin));
		}
		else if ( pBot->FInViewCone(m_pFriendly) )
		{
			CBotWeapon *pWeapon = pBot->getBestWeapon(NULL,true,true);

			if ( pWeapon )
			{
				pBot->selectBotWeapon(pWeapon);

				if ( randomInt(0,1) )
					pBot->primaryAttack();
			}
		}

		if ( !m_fTime )
			m_fTime = engine->Time() + randomFloat(3.5f,8.0f);
	}
	break;// taunt at my friendly player
	case 1:
	{
		Vector origin = CBotGlobals::entityOrigin(m_pFriendly);
		bool ok = true;

		if ( !pBot->FInViewCone(m_pFriendly) )
		{
			pBot->setLookVector(origin);
			pBot->setLookAtTask((LOOK_VECTOR));
			ok = false;
		}

		if ( pBot->distanceFrom(m_pFriendly) > 100 )
		{
			pBot->setMoveTo((origin));
			ok = false;
		}

		if ( ok )
		{
			if ( pBot->isTF2() )
				((CBotTF2*)pBot)->taunt();
			//else if ( pBot->isDOD() )
			//	((CDODBot*)pBot)->taunt(); pBot->impulse(100);
		}

		if ( !m_fTime )
			m_fTime = engine->Time() + randomFloat(3.5f,6.5f);

	}
	// say some random voice commands
	break;
	case 2:
	{
		if ( !m_fTime )
			pBot->addVoiceCommand(randomInt(0,m_iMaxVoiceCmd-1));

		if ( !m_fTime )
			m_fTime = engine->Time() + randomFloat(1.5f,3.0f);
	}
	// press some random buttons, such as attack2, jump
	break;
	case 3:
	{
		if ( randomInt(0,1) )
			pBot->jump();
		else
		{
			if ( pBot->isTF2() )
			{
				if ( ((CBotTF2*)pBot)->getClass() == TF_CLASS_HWGUY )
					pBot->secondaryAttack(true);
			}
		}

		if ( !m_fTime )
			m_fTime = engine->Time() + randomFloat(1.5f,3.0f);
	}
	default:
		break;
	}

	if ( m_fTime < engine->Time() )
		complete();

	if ( pBot->isTF2() )
	{
		if ( CTeamFortress2Mod::hasRoundStarted() )
			complete();
	}
	else if ( pBot->isDOD() )
	{
		if ( CDODMod::m_Flags.getNumFlags() > 0 )
			complete();
	}

}

/////////////

void CBotNest :: execute (CBot *pBot, CBotSchedule *pSchedule)
{
	CBotTF2 *pBotTF2 = (CBotTF2*)pBot;

	if ( pBotTF2->someoneCalledMedic() )
		fail();

	if ( !pBotTF2->wantToNest() )
	{
		complete();
		pBotTF2->addVoiceCommand(TF_VC_GOGOGO);
		return;
	}
	else if ( pBot->hasSomeConditions(CONDITION_PUSH) )
	{
		complete();
		pBot->removeCondition(CONDITION_PUSH);
		pBotTF2->addVoiceCommand(TF_VC_GOGOGO);
		return;
	}

	if ( m_fTime == 0 )
	{
		m_fTime = engine->Time() + randomFloat(5.0,10.0);

		if ( randomInt(0,1) )
			pBotTF2->addVoiceCommand(TF_VC_HELP);
	}
	else if ( m_fTime < engine->Time() )
	{
		complete();
		pBotTF2->addVoiceCommand(TF_VC_GOGOGO);
	}

	// wait around
	// wait for more friendlies
	// heal up
	// 

	pBot->setLookAtTask((LOOK_AROUND));

	pBot->stopMoving();

}

CBotNest::CBotNest()
{
	m_fTime = 0.0f;
	m_pEnemy = NULL;
}
////////////////////////////////////////////////


////////////////////////////////////////////////////

CBotDODSnipe :: CBotDODSnipe ( CBotWeapon *pWeaponToUse, Vector vOrigin, float fYaw, bool bUseZ, float z, int iWaypointType )
{
	QAngle angle;
	m_fEnemyTime = 0.0f;
	m_fTime = 0.0f;
	angle = QAngle(0,fYaw,0);
	AngleVectors(angle,&m_vAim);
	m_vAim = vOrigin + (m_vAim*1024);
	m_vOrigin = vOrigin;
	m_pWeaponToUse = pWeaponToUse;
	m_fScopeTime = 0;
	m_bUseZ = bUseZ;
	m_z = z; // z = ground level
	m_iWaypointType = iWaypointType;
}

void CBotDODSnipe :: debugString ( char *string )
{
	sprintf(string,"CBotDODSnipe\nm_fTime = %0.2f\npWeaponToUse = %s\nm_bUseZ = %s\nm_z = %0.2f",m_fTime,m_pWeaponToUse->getWeaponInfo()->getWeaponName(),m_bUseZ ? "true":"false",m_z);
}
	
void CBotDODSnipe :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	static CBotWeapon *pCurrentWeapon;
	static CWeapon *pWeapon;

	static bool bDeployedOrZoomed;
	static float fDist;

	bDeployedOrZoomed = false;

	pBot->wantToShoot(false);
	pBot->wantToListen(false);

	if ( m_fTime == 0.0f )
	{
		m_fEnemyTime = engine->Time();
		m_fTime = m_fEnemyTime + randomFloat(20.0f,40.0f);
		pBot->resetLookAroundTime();
	}

	pCurrentWeapon = pBot->getCurrentWeapon();

	if ( !pCurrentWeapon )
	{
		fail();
		return;
	}

	pWeapon = pCurrentWeapon->getWeaponInfo();

	if (pWeapon == NULL)
	{
		fail();
		return;
	}

	if ( pCurrentWeapon != m_pWeaponToUse )
	{
		if ( !pBot->select_CWeapon(CWeapons::getWeapon(m_pWeaponToUse->getID())) )
		{
			fail();
		}

		return;
	}
	else
	{
		if ( pCurrentWeapon->isZoomable() )
			bDeployedOrZoomed = CClassInterface::isSniperWeaponZoomed(pCurrentWeapon->getWeaponEntity());
		else if ( pCurrentWeapon->isDeployable() )
			bDeployedOrZoomed = CClassInterface::isMachineGunDeployed(pCurrentWeapon->getWeaponEntity());

		if ( m_fScopeTime < engine->Time() )
		{
			if ( !bDeployedOrZoomed )
			{
				pBot->secondaryAttack();
			}

			m_fScopeTime = engine->Time() + randomFloat(0.5f,1.0f);
		}
	}

	if ( pCurrentWeapon->getAmmo(pBot) < 1 )
	{
		if ( bDeployedOrZoomed )
			pBot->secondaryAttack();

		complete();
	}
	else if ( pBot->distanceFrom(m_vOrigin) > 200 ) // too far from sniper point
	{
		if ( bDeployedOrZoomed )
			pBot->secondaryAttack();
		// too far away
		fail();
	}

	if ( (m_fEnemyTime + 5.0f) > engine->Time() )
	{
		pBot->setLookAtTask(LOOK_VECTOR);
		pBot->setLookVector(m_vLastEnemy);
	}
	else if ( m_bUseZ )
	{
		Vector vAim = Vector(m_vAim.x,m_vAim.y,m_z);
		pBot->setLookAtTask(LOOK_VECTOR);
		pBot->setLookVector(pBot->snipe(vAim));
	}
	else
	{
		pBot->setLookAtTask(LOOK_SNIPE);
		pBot->setLookVector(pBot->snipe(m_vAim));
	}

	fDist = (m_vOrigin - pBot->getOrigin()).Length2D();

	if ( (fDist > 16) || !bDeployedOrZoomed )
	{
		pBot->setMoveTo(m_vOrigin);
		pBot->setMoveSpeed(CClassInterface::getMaxSpeed(pBot->getEdict())/8);

		if ( ( fDist < 48 ) && ((CDODBot*)pBot)->withinTeammate() )
			fail();
	}
	else
	{
		pBot->stopMoving();

		if ( m_iWaypointType & CWaypointTypes::W_FL_PRONE )
		{			
			//pBot->updateDanger(MAX_BELIEF);
			pBot->removeCondition(CONDITION_RUN);
			pBot->updateCondition(CONDITION_PRONE);
		}
		else
		{
			if ( m_iWaypointType & CWaypointTypes::W_FL_CROUCH )
				pBot->duck();
			// refrain from proning
			pBot->updateCondition(CONDITION_RUN);
			pBot->removeCondition(CONDITION_PRONE);
		}

		// no enemy for a while
		if ( (m_fEnemyTime + m_fTime) < engine->Time() )
		{
			if ( bDeployedOrZoomed )
				pBot->secondaryAttack();

			complete();
		}
	}

	if ( pBot->hasEnemy() )
	{
		pBot->setMoveLookPriority(MOVELOOK_ATTACK);

		pBot->setLookAtTask(LOOK_ENEMY);

		pBot->handleAttack(pCurrentWeapon,pBot->getEnemy());

		pBot->setMoveLookPriority(MOVELOOK_TASK);

		// havin' fun
		m_fEnemyTime = engine->Time();

		m_vLastEnemy = CBotGlobals::entityOrigin(pBot->getEnemy());
	}
}

//////////////////////////

CBotHL2DMSnipe :: CBotHL2DMSnipe ( CBotWeapon *pWeaponToUse, Vector vOrigin, float fYaw, bool bUseZ, float z, int iWaypointType )
{
	QAngle angle;
	m_fEnemyTime = 0.0f;
	m_fTime = 0.0f;
	angle = QAngle(0,fYaw,0);
	AngleVectors(angle,&m_vAim);
	m_vAim = vOrigin + (m_vAim*1024);
	m_vOrigin = vOrigin;
	m_pWeaponToUse = pWeaponToUse;
	m_fScopeTime = 0;
	m_bUseZ = bUseZ;
	m_z = z; // z = ground level
	m_iWaypointType = iWaypointType;
}

void CBotHL2DMSnipe :: debugString ( char *string )
{
	sprintf(string,"CBotHL2DMSnipe\nm_fTime = %0.2f\npWeaponToUse = %s\nm_bUseZ = %s\nm_z = %0.2f",m_fTime,m_pWeaponToUse->getWeaponInfo()->getWeaponName(),m_bUseZ ? "true":"false",m_z);
}

void CBotHL2DMSnipe :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	static CBotWeapon *pCurrentWeapon;
	static CWeapon *pWeapon;

	static bool bDeployedOrZoomed;
	static float fDist;

	bDeployedOrZoomed = false;

	pBot->wantToShoot(false);
	pBot->wantToListen(false);

	if ( m_fTime == 0.0f )
	{
		m_fEnemyTime = engine->Time();
		m_fTime = m_fEnemyTime + randomFloat(20.0f,40.0f);
		pBot->resetLookAroundTime();
	}

	pCurrentWeapon = pBot->getCurrentWeapon();

	if ( !pCurrentWeapon )
	{
		fail();
		return;
	}

	pWeapon = pCurrentWeapon->getWeaponInfo();

	if (pWeapon == NULL)
	{
		fail();
		return;
	}

	// refrain from proning
	pBot->updateCondition(CONDITION_RUN);

	if ( pCurrentWeapon != m_pWeaponToUse )
	{
		if ( !pBot->select_CWeapon(CWeapons::getWeapon(m_pWeaponToUse->getID())) )
		{
			fail();
		}

		return;
	}

	if ( pCurrentWeapon->getAmmo(pBot) < 1 )
	{
		complete();
	}
	else if ( pBot->distanceFrom(m_vOrigin) > 200 ) // too far from sniper point
	{
		// too far away
		fail();
	}

	if ( m_bUseZ )
	{
		Vector vAim = Vector(m_vAim.x,m_vAim.y,m_z);
		pBot->setLookAtTask(LOOK_VECTOR);
		pBot->setLookVector(pBot->snipe(vAim));
	}
	else
	{
		pBot->setLookAtTask(LOOK_SNIPE);
		pBot->setLookVector(pBot->snipe(m_vAim));
	}

	fDist = (m_vOrigin - pBot->getOrigin()).Length2D();

	if ( fDist > 16 )
	{
		pBot->setMoveTo(m_vOrigin);
		pBot->setMoveSpeed(CClassInterface::getMaxSpeed(pBot->getEdict())/8);

		//if ( ( fDist < 48 ) && ((CDODBot*)pBot)->withinTeammate() )
		//	fail();
	}
	else
	{
		pBot->stopMoving();

		if ( m_iWaypointType & CWaypointTypes::W_FL_CROUCH )
			pBot->duck();

		// no enemy for a while
		if ( (m_fEnemyTime + m_fTime) < engine->Time() )
		{
			//if ( bDeployedOrZoomed )
			//	pBot->secondaryAttack();

			complete();
		}
	}

	if ( pBot->hasEnemy() )
	{
		pBot->setMoveLookPriority(MOVELOOK_ATTACK);
		pBot->setLookAtTask(LOOK_ENEMY);
		pBot->handleAttack(pCurrentWeapon,pBot->getEnemy());
		pBot->setMoveLookPriority(MOVELOOK_TASK);

		// havin' fun
		m_fEnemyTime = engine->Time();
	}
}


////////////////////////////////////////////////////////////////////////////////////////////
// Base Task
CBotTask :: CBotTask ()
{
	_init();
}

bool CBotTask :: timedOut ()
{
	return (this->m_fTimeOut != 0) && (engine->Time() < this->m_fTimeOut);
}

eTaskState CBotTask :: isInterrupted (CBot *pBot)
{
	if ( m_iCompleteInterruptConditionsHave )
	{
		if ( pBot->hasSomeConditions(m_iCompleteInterruptConditionsHave) )
			return STATE_COMPLETE;
	}

	if ( m_iCompleteInterruptConditionsDontHave )
	{
		if ( !pBot->hasAllConditions(m_iCompleteInterruptConditionsDontHave) )
			return STATE_COMPLETE;
	}

	if ( m_iFailInterruptConditionsHave )
	{
		if ( pBot->hasSomeConditions(m_iFailInterruptConditionsHave) )
			return STATE_FAIL;
	}

	if ( m_iFailInterruptConditionsDontHave )
	{
		if ( !pBot->hasAllConditions(m_iFailInterruptConditionsDontHave) )
			return STATE_FAIL;
	}

	return STATE_RUNNING;
}

void CBotTask :: _init()
{
	m_iFlags = 0;	
	m_iState = STATE_IDLE;
	m_fTimeOut = 0;
//	m_pEdict = NULL;
//	m_fFloat = 0;
//	m_iInt = 0;
//	m_vVector = Vector(0,0,0);
	m_iFailInterruptConditionsHave = 0;
	m_iFailInterruptConditionsDontHave = 0;
	m_iCompleteInterruptConditionsHave = 0;
	m_iCompleteInterruptConditionsDontHave = 0;
	init();
}

void CBotTask :: init ()
{
	return;
}

void CBotTask :: execute ( CBot *pBot, CBotSchedule *pSchedule )
{
	return;
}

bool CBotTask :: hasFailed ()
{
	return m_iState == STATE_FAIL;
}

bool CBotTask :: isComplete ()
{
	return m_iState == STATE_COMPLETE;
}
/*
void CBotTask :: setVector ( Vector vOrigin )
{
	m_vVector = vOrigin;
}

void CBotTask :: setFloat ( float fFloat )
{
	m_fFloat = fFloat;
}

void CBotTask :: setEdict ( edict_t *pEdict )
{
	m_pEdict = pEdict;
}
*/
// if this condition is true it will complete, if bUnset is true, the condition must be false to be complete
void CBotTask :: setCompleteInterrupt ( int iInterruptHave, int iInterruptDontHave )
{
	m_iCompleteInterruptConditionsHave = iInterruptHave;
	m_iCompleteInterruptConditionsDontHave = iInterruptDontHave;
}

void CBotTask :: setFailInterrupt ( int iInterruptHave, int iInterruptDontHave )
{
	m_iFailInterruptConditionsHave = iInterruptHave;
	m_iFailInterruptConditionsDontHave = iInterruptDontHave;
}

void CBotTask :: fail ()
{
	m_iState = STATE_FAIL;
}

void CBotTask :: complete ()
{
	m_iState = STATE_COMPLETE;
}
