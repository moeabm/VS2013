//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "cbase.h"
#include "weapon_sdkbase.h"
#include "sdk_weapon_melee.h"

#if defined( CLIENT_DLL )

	#define CWeaponPoolCue C_WeaponPoolCue
	#include "c_sdk_player.h"

#else

	#include "sdk_player.h"

#endif


class CWeaponPoolCue : public CWeaponSDKMelee
{
public:
	DECLARE_CLASS( CWeaponPoolCue, CWeaponSDKMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();
	
	CWeaponPoolCue();

	virtual SDKWeaponID GetWeaponID( void ) const		{	return SDK_WEAPON_POOLCUE; }
	virtual float	GetRange( void )					{	return	64.0f;	}	//Tony; let the poolcue swing further.
	virtual bool CanWeaponBeDropped() const				{	return false; }
	virtual void SecondaryAttack();

private:
	
	CWeaponPoolCue( const CWeaponPoolCue & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponPoolCue, DT_WeaponPoolCue )

BEGIN_NETWORK_TABLE( CWeaponPoolCue, DT_WeaponPoolCue )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponPoolCue )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_poolcue, CWeaponPoolCue );
PRECACHE_WEAPON_REGISTER( weapon_poolcue );



CWeaponPoolCue::CWeaponPoolCue()
{
}


void CWeaponPoolCue::SecondaryAttack()
{
	CSDKPlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		return;
	
	WeaponSound( SINGLE );

	
	trace_t traceHit;


	Vector swingStart = pPlayer->Weapon_ShootPosition( );
	Vector forward;

	pPlayer->EyeVectors( &forward, NULL, NULL );

	Vector swingEnd = swingStart + forward * GetRange();
	UTIL_TraceLine( swingStart, swingEnd, MASK_SHOT_HULL, pPlayer, COLLISION_GROUP_NONE, &traceHit );
	Activity nHitActivity = ACT_VM_HITCENTER;

	Msg("Secondary PoolCue swing\n");
	
	//TODO: add knockback code
	
	// -------------------------
	//	Miss
	// -------------------------
	if ( traceHit.fraction != 1.0f )
	{
		nHitActivity = ACT_VM_HITCENTER;
		CSDKPlayer *pHitPlayer = (CSDKPlayer *)traceHit.m_pEnt;
		
		if ( pHitPlayer != NULL && pHitPlayer->IsPlayer() && pHitPlayer->GetTeamNumber() == SDK_TEAM_BLUE)
		{
			Vector hitDirection;
			pPlayer->EyeVectors( &hitDirection, NULL, NULL );
			VectorNormalize( hitDirection );

			Vector hitForce = hitDirection * 700.0f;
			
			pHitPlayer->SetAbsOrigin(pHitPlayer->GetAbsOrigin() + Vector(0,0,10));
			pHitPlayer->SetBaseVelocity(hitForce);

			
#ifndef CLIENT_DLL
			CTakeDamageInfo info( GetOwner(), GetOwner(), 10.0f, DMG_CLUB);
			info.SetDamageForce(hitForce);
			pHitPlayer->TakeDamage(info);
#endif
		}
		
		WeaponSound( MELEE_HIT );
	}
	else
	{
		nHitActivity = ACT_VM_MISSCENTER;
	}

	// Send the anim
	SendWeaponAnim( nHitActivity );

	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_SECONDARY );

	//Setup our next attack times
	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	m_flNextSecondaryAttack = gpGlobals->curtime + GetFireRate();
}

//Tony; todo; add ACT_MP_PRONE* activities, so we have them.
acttable_t CWeaponPoolCue::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,					ACT_DOD_STAND_AIM_SPADE,				false },
	{ ACT_MP_CROUCH_IDLE,					ACT_DOD_CROUCH_AIM_SPADE,				false },
	{ ACT_MP_PRONE_IDLE,					ACT_DOD_PRONE_AIM_SPADE,				false },

	{ ACT_MP_RUN,							ACT_DOD_RUN_AIM_SPADE,					false },
	{ ACT_MP_WALK,							ACT_DOD_WALK_AIM_SPADE,					false },
	{ ACT_MP_CROUCHWALK,					ACT_DOD_CROUCHWALK_AIM_SPADE,			false },
	{ ACT_MP_PRONE_CRAWL,					ACT_DOD_PRONEWALK_AIM_SPADE,			false },
	{ ACT_SPRINT,							ACT_DOD_SPRINT_AIM_SPADE,				false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_DOD_PRIMARYATTACK_SPADE,			false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_DOD_PRIMARYATTACK_SPADE,			false },
	{ ACT_MP_ATTACK_PRONE_PRIMARYFIRE,		ACT_DOD_PRIMARYATTACK_PRONE_SPADE,		false },
};

IMPLEMENT_ACTTABLE( CWeaponPoolCue );

