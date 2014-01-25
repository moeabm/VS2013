//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "cbase.h"
#include "weapon_sdkbase.h"
#include "sdk_weapon_melee.h"

#if defined( CLIENT_DLL )

	#define CWeaponClaws2 C_WeaponClaws2
	#include "c_sdk_player.h"

#else

	#include "sdk_player.h"

#endif


class CWeaponClaws2 : public CWeaponSDKMelee
{
public:
	DECLARE_CLASS( CWeaponClaws2, CWeaponSDKMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();
	
	CWeaponClaws2();

	virtual SDKWeaponID GetWeaponID( void ) const		{	return SDK_WEAPON_CLAWS2; }
	virtual float	GetRange( void )					{	return	64.0f;	}	//Tony; let the claws2 swing further.
	virtual float	GetCloackLenght( void )					{	return	5.0f;	}	//Tony; let the claws2 swing further.
	virtual bool CanWeaponBeDropped() const				{	return false; }
	void SecondaryAttack();
	void GoInvisible();


private:
	bool isInvisible;
	float startCloackTime;
	CWeaponClaws2( const CWeaponClaws2 & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponClaws2, DT_WeaponClaws2 )

BEGIN_NETWORK_TABLE( CWeaponClaws2, DT_WeaponClaws2 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponClaws2 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_claws2, CWeaponClaws2 );
PRECACHE_WEAPON_REGISTER( weapon_claws2 );



CWeaponClaws2::CWeaponClaws2()
{
}

//Tony; todo; add ACT_MP_PRONE* activities, so we have them.
acttable_t CWeaponClaws2::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,					ACT_VS_STAND_IDLE_CLAWS,				false },
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

IMPLEMENT_ACTTABLE( CWeaponClaws2 );

void CWeaponClaws2::SecondaryAttack()
{
	CSDKPlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		return;
	
	
	WeaponSound( SPECIAL1 );

	Msg("Secondary Claws2 swing\n");
	
	startCloackTime = gpGlobals->curtime;
	
	pPlayer->GoInvisible(5.0f);
	m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();
}