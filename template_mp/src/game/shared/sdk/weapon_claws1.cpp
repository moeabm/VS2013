//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "cbase.h"
#include "weapon_sdkbase.h"
#include "sdk_weapon_melee.h"

#if defined( CLIENT_DLL )

	#define CWeaponClaws1 C_WeaponClaws1
	#include "c_sdk_player.h"
	#include "c_sdk_team.h"

#else

	#include "sdk_player.h"

#endif


class CWeaponClaws1 : public CWeaponSDKMelee
{
public:
	DECLARE_CLASS( CWeaponClaws1, CWeaponSDKMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();
	
	CWeaponClaws1();

	virtual SDKWeaponID GetWeaponID( void ) const		{	return SDK_WEAPON_CLAWS1; }
	virtual float	GetRange( void )					{	return	64.0f;	}	//Tony; let the claws1 swing further.
	virtual float	GetCloackLenght( void )					{	return	5.0f;	}	//Tony; let the claws1 swing further.
	virtual bool CanWeaponBeDropped() const				{	return false; }
	void SecondaryAttack();


private:
	CWeaponClaws1( const CWeaponClaws1 & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponClaws1, DT_WeaponClaws1 )

BEGIN_NETWORK_TABLE( CWeaponClaws1, DT_WeaponClaws1 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponClaws1 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_claws1, CWeaponClaws1 );
PRECACHE_WEAPON_REGISTER( weapon_claws1 );



CWeaponClaws1::CWeaponClaws1()
{
}

//Tony; todo; add ACT_MP_PRONE* activities, so we have them.
acttable_t CWeaponClaws1::m_acttable[] = 
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

IMPLEMENT_ACTTABLE( CWeaponClaws1 );

void CWeaponClaws1::SecondaryAttack()
{
	CSDKPlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		return;
	
	const FileWeaponInfo_t *pWeaponInfo = &GetWpnData();
	const CSDKWeaponInfo *pSDKInfo;
 
#ifdef _DEBUG
	pSDKInfo = dynamic_cast< const CSDKWeaponInfo* >( pWeaponInfo );
	Assert( pSDKInfo );
#else
	pSDKInfo = static_cast< const CSDKWeaponInfo* >( pWeaponInfo );
#endif
 
#if defined( CLIENT_DLL )
	WeaponSound( SPECIAL1 );
	C_SDKTeam *iTeam = GetGlobalSDKTeam( SDK_TEAM_RED );
	for( int i =0 ; i < iTeam->Get_Number_Players() ; i++){
		CSDKPlayer *tmpPlayer = dynamic_cast< CSDKPlayer* > (iTeam->GetPlayer(i));
		if(tmpPlayer->IsAlive()){
			tmpPlayer->Glow(pSDKInfo->m_flSpecialActive);
		}
	}
#else
	pPlayer->GoStealth(pSDKInfo->m_flSpecialActive);
#endif
	m_flEndSecondaryAttack = gpGlobals->curtime + pSDKInfo->m_flSpecialActive;
	m_flNextSecondaryAttack = gpGlobals->curtime + pSDKInfo->m_iSpecialCooldown;
	BaseClass::SecondaryAttack();
}