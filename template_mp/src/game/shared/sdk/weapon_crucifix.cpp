//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "cbase.h"
#include "weapon_sdkbase.h"
#include "sdk_weapon_melee.h"

#if defined( CLIENT_DLL )

	#define CWeaponCrucifix C_WeaponCrucifix
	#include "c_sdk_player.h"

#else

	#include "sdk_player.h"

#endif


class CWeaponCrucifix : public CWeaponSDKMelee
{
public:
	DECLARE_CLASS( CWeaponCrucifix, CWeaponSDKMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();
	
	CWeaponCrucifix();

	virtual SDKWeaponID GetWeaponID( void ) const		{	return SDK_WEAPON_CRUCIFIX; }
	virtual float	GetRange( void )					{	return	64.0f;	}	//Tony; let the crucifix swing further.
	virtual bool CanWeaponBeDropped() const				{	return false; }
	virtual void SecondaryAttack();

private:
	
	float startPrayTime;
	CWeaponCrucifix( const CWeaponCrucifix & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponCrucifix, DT_WeaponCrucifix )

BEGIN_NETWORK_TABLE( CWeaponCrucifix, DT_WeaponCrucifix )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponCrucifix )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_crucifix, CWeaponCrucifix );
PRECACHE_WEAPON_REGISTER( weapon_crucifix );



CWeaponCrucifix::CWeaponCrucifix()
{
}


void CWeaponCrucifix::SecondaryAttack()
{
	CSDKPlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		return;

	WeaponSound( SPECIAL1 );

	Msg("Secondary Crucifix swing\n");
	
	startPrayTime = gpGlobals->curtime;
	//Possible future feature: Have the praying father d glow white
	//TODO: add pray glow

#ifndef CLIENT_DLL // right now pray function is server only
	pPlayer->Pray(3.0f);
#endif
	m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();
}

//Tony; todo; add ACT_MP_PRONE* activities, so we have them.
acttable_t CWeaponCrucifix::m_acttable[] = 
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

IMPLEMENT_ACTTABLE( CWeaponCrucifix );

