//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "cbase.h"
#include "weapon_sdkbase.h"
#include "sdk_weapon_melee.h"

#include "sdk_fx_shared.h"
#if defined( CLIENT_DLL )

	#define CWeaponColt C_WeaponColt
	#include "c_sdk_player.h"

#else

	#include "sdk_player.h"

#endif


class CWeaponColt : public CWeaponSDKMelee
{
public:
	DECLARE_CLASS( CWeaponColt, CWeaponSDKMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();
	
	CWeaponColt();

	virtual SDKWeaponID GetWeaponID( void ) const		{	return SDK_WEAPON_COLT; }
	virtual float	GetRange( void )					{	return	64.0f;	}	//Tony; let the colt swing further.
	virtual bool CanWeaponBeDropped() const				{	return false; }
	virtual void SecondaryAttack();
	virtual void ItemPostFrame();

private:

	CWeaponColt( const CWeaponColt & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponColt, DT_WeaponColt )

BEGIN_NETWORK_TABLE( CWeaponColt, DT_WeaponColt )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponColt )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_colt, CWeaponColt );
PRECACHE_WEAPON_REGISTER( weapon_colt );



CWeaponColt::CWeaponColt()
{
}

void CWeaponColt::SecondaryAttack()
{
	CSDKPlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		return;

	if( m_bInReload ) return;
		// If my clip is empty (and I use clips) start reload
	if ( UsesClipsForAmmo1() && m_iClip1 < 1 ) 
	{
		m_bInReload = true;
		Reload();
		return;
	}

	
 
#ifdef GAME_DLL
	pPlayer->NoteWeaponFired();
#endif
 
	pPlayer->DoMuzzleFlash();
 
	SendWeaponAnim( GetSecondaryAttackActivity() );
 
	// Make sure we don't fire more than the amount in the clip
	if ( UsesClipsForAmmo1() )
		m_iClip1 --;
	else
		pPlayer->RemoveAmmo(1, m_iPrimaryAmmoType );
 
	pPlayer->IncreaseShotsFired();
 
	float flSpread = GetWeaponSpread();
 
	FX_FireBullets(
		pPlayer->entindex(),
		pPlayer->Weapon_ShootPosition(),
		pPlayer->EyeAngles() + pPlayer->GetPunchAngle(),
		GetWeaponID(),
		0, //Tony; fire mode - this is unused at the moment, left over from CSS when SDK* was created in the first place.
		CBaseEntity::GetPredictionRandomSeed() & 255,
		flSpread
		);
 
    //Hack: override the swing sound from above
	WeaponSound( SPECIAL1 );

	//Add our view kick in
	AddViewKick();
 
	bool doPunch = true;
	if ( m_iClip1 <= 0 )
		doPunch = false;

	if ( doPunch )
	{

		// Update punch angles.
		QAngle angle = pPlayer->GetPunchAngle();

		if ( pPlayer->GetFlags() & FL_ONGROUND )
		{
			angle.x -= SharedRandomInt( "ShotgunPunchAngleGround", 2, 3 );
		}
		else
		{
			angle.x -= SharedRandomInt( "ShotgunPunchAngleAir", 4, 6 );
		}

		pPlayer->SetPunchAngle( angle );
	}

	//Tony; update our weapon idle time
	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
	
	m_flNextPrimaryAttack = gpGlobals->curtime +  GetFireRate();
	m_flNextSecondaryAttack = gpGlobals->curtime +  GetFireRate();
	BaseClass::SecondaryAttack();
	
}

void CWeaponColt::ItemPostFrame( void )
{
	//Check Reload
	if ( UsesClipsForAmmo1() )
		CheckReload();
	BaseClass::ItemPostFrame();
}

//Tony; todo; add ACT_MP_PRONE* activities, so we have them.
acttable_t CWeaponColt::m_acttable[] = 
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
	
	{ ACT_MP_RELOAD_STAND,					ACT_DOD_RELOAD_PISTOL,					false },
	{ ACT_MP_RELOAD_CROUCH,					ACT_DOD_RELOAD_CROUCH_PISTOL,			false },
	{ ACT_MP_RELOAD_PRONE,					ACT_DOD_RELOAD_PRONE_PISTOL,			false },
};

IMPLEMENT_ACTTABLE( CWeaponColt );

