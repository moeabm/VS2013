//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "cbase.h"
#include "weapon_sdkbase.h"

#if defined( CLIENT_DLL )

	#define CWeaponUzi C_WeaponUzi
	#include "c_sdk_player.h"

#else

	#include "sdk_player.h"

#endif


class CWeaponUzi : public CWeaponSDKBase
{
public:
	DECLARE_CLASS( CWeaponUzi, CWeaponSDKBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();
	
	CWeaponUzi();

	virtual SDKWeaponID GetWeaponID( void ) const		{ return SDK_WEAPON_UZI; }
	virtual int GetFireMode() const { return FM_AUTOMATIC; }

private:

	CWeaponUzi( const CWeaponUzi & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponUzi, DT_WeaponUzi )

BEGIN_NETWORK_TABLE( CWeaponUzi, DT_WeaponUzi )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponUzi )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_uzi, CWeaponUzi );
PRECACHE_WEAPON_REGISTER( weapon_uzi );



CWeaponUzi::CWeaponUzi()
{
}

//Tony; todo; add ACT_MP_PRONE* activities, so we have them.
acttable_t CWeaponUzi::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,					ACT_DOD_STAND_IDLE_PISTOL,				false },
	{ ACT_MP_CROUCH_IDLE,					ACT_DOD_CROUCH_IDLE_PISTOL,				false },
	{ ACT_MP_PRONE_IDLE,					ACT_DOD_PRONE_AIM_PISTOL,				false },

	{ ACT_MP_RUN,							ACT_DOD_RUN_AIM_PISTOL,					false },
	{ ACT_MP_WALK,							ACT_DOD_WALK_AIM_PISTOL,				false },
	{ ACT_MP_CROUCHWALK,					ACT_DOD_CROUCHWALK_AIM_PISTOL,			false },
	{ ACT_MP_PRONE_CRAWL,					ACT_DOD_PRONEWALK_IDLE_PISTOL,			false },
	{ ACT_SPRINT,							ACT_DOD_SPRINT_IDLE_PISTOL,				false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_DOD_PRIMARYATTACK_PISTOL,			false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_DOD_PRIMARYATTACK_PISTOL,			false },
	{ ACT_MP_ATTACK_PRONE_PRIMARYFIRE,		ACT_DOD_PRIMARYATTACK_PRONE_PISTOL,		false },

	{ ACT_MP_RELOAD_STAND,					ACT_DOD_RELOAD_PISTOL,					false },
	{ ACT_MP_RELOAD_CROUCH,					ACT_DOD_RELOAD_CROUCH_PISTOL,			false },
	{ ACT_MP_RELOAD_PRONE,					ACT_DOD_RELOAD_PRONE_PISTOL,			false },
};

IMPLEMENT_ACTTABLE( CWeaponUzi );

