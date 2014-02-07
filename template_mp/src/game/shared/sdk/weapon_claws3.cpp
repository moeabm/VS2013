//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "cbase.h"
#include "weapon_sdkbase.h"
#include "sdk_weapon_melee.h"

#if defined( CLIENT_DLL )

	#define CWeaponClaws3 C_WeaponClaws3
	#include "c_sdk_player.h"

#else

	#include "sdk_player.h"

#endif


class CWeaponClaws3 : public CWeaponSDKMelee
{
public:
	DECLARE_CLASS( CWeaponClaws3, CWeaponSDKMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();
	
	CWeaponClaws3();

	virtual SDKWeaponID GetWeaponID( void ) const		{	return SDK_WEAPON_CLAWS3; }
	virtual float	GetRange( void )					{	return	64.0f;	}	//Tony; let the claws3 swing further.
	virtual float	GetCloackLenght( void )					{	return	5.0f;	}	//Tony; let the claws3 swing further.
	virtual bool CanWeaponBeDropped() const				{	return false; }
	void SecondaryAttack();


private:
	CWeaponClaws3( const CWeaponClaws3 & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponClaws3, DT_WeaponClaws3 )

BEGIN_NETWORK_TABLE( CWeaponClaws3, DT_WeaponClaws3 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponClaws3 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_claws3, CWeaponClaws3 );
PRECACHE_WEAPON_REGISTER( weapon_claws3 );



CWeaponClaws3::CWeaponClaws3()
{
}

//Tony; todo; add ACT_MP_PRONE* activities, so we have them.
acttable_t CWeaponClaws3::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,					ACT_VS_STAND_IDLE_CLAWS,				false },
	{ ACT_MP_CROUCH_IDLE,					ACT_DOD_CROUCH_AIM_SPADE,				false },
	{ ACT_MP_PRONE_IDLE,					ACT_DOD_PRONE_AIM_SPADE,				false },

	{ ACT_MP_RUN,							ACT_DOD_RUN_AIM_SPADE,					false },
	{ ACT_MP_WALK,							ACT_DOD_WALK_AIM_SPADE,					false },
	{ ACT_MP_CROUCHWALK,					ACT_DOD_CROUCHWALK_AIM_SPADE,			false },
	{ ACT_MP_PRONE_CRAWL,					ACT_DOD_PRONEWALK_AIM_SPADE,			false },
	{ ACT_SPRINT,							ACT_DOD_SPRINT_AIM_SPADE,				false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_VS_ATTACK_CLAWS,			false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_DOD_PRIMARYATTACK_SPADE,			false },
	{ ACT_MP_ATTACK_PRONE_PRIMARYFIRE,		ACT_DOD_PRIMARYATTACK_PRONE_SPADE,		false },
};

IMPLEMENT_ACTTABLE( CWeaponClaws3 );

void CWeaponClaws3::SecondaryAttack()
{
	CSDKPlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		return;
	

	WeaponSound( SPECIAL1 );

	Msg("Secondary Claws3 swing\n");
	
#if defined ( CLIENT_DLL )
#else
	pPlayer->GoSunImmune(5.0f);
#endif

	m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();
}