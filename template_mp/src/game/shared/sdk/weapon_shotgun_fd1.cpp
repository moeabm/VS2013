//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_sdkbase.h"
#include "sdk_fx_shared.h"


#if defined( CLIENT_DLL )

	#define CWeaponShotgunFD1 C_WeaponShotgunFD1
	#include "c_sdk_player.h"

#else

	#include "sdk_player.h"
	#include "te_firebullets.h"

#endif


class CWeaponShotgunFD1 : public CWeaponSDKBase
{
public:
	DECLARE_CLASS( CWeaponShotgunFD1, CWeaponSDKBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponShotgunFD1();

	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual bool Reload();
	virtual void WeaponIdle();

	virtual SDKWeaponID GetWeaponID( void ) const		{ return SDK_WEAPON_SHOTGUN_FD1; }
	virtual float GetWeaponSpread() { return 0.05362f; }


private:

	CWeaponShotgunFD1( const CWeaponShotgunFD1 & );

	float m_flPumpTime;
	CNetworkVar( int, m_iInSpecialReload );

};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponShotgunFD1, DT_WeaponShotgunFD1 )

BEGIN_NETWORK_TABLE( CWeaponShotgunFD1, DT_WeaponShotgunFD1 )

	#ifdef CLIENT_DLL
		RecvPropInt( RECVINFO( m_iInSpecialReload ) )
	#else
		SendPropInt( SENDINFO( m_iInSpecialReload ), 2, SPROP_UNSIGNED )
	#endif

END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponShotgunFD1 )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_iInSpecialReload, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_shotgun_fd1, CWeaponShotgunFD1 );
PRECACHE_WEAPON_REGISTER( weapon_shotgun_fd1 );



CWeaponShotgunFD1::CWeaponShotgunFD1()
{
	m_flPumpTime = 0;
}

void CWeaponShotgunFD1::PrimaryAttack()
{
	CSDKPlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		return;

	// don't fire underwater
	if (pPlayer->GetWaterLevel() == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.15;
		return;
	}
	bool doPunch = true;
	if ( m_iClip1 <= 0 )
		doPunch = false;

	BaseClass::PrimaryAttack();

	if ( doPunch )
	{
		m_iInSpecialReload = 0;

		// Update punch angles.
		QAngle angle = pPlayer->GetPunchAngle();

		if ( pPlayer->GetFlags() & FL_ONGROUND )
		{
			angle.x -= SharedRandomInt( "ShotgunPunchAngleGround", 4, 6 );
		}
		else
		{
			angle.x -= SharedRandomInt( "ShotgunPunchAngleAir", 8, 11 );
		}

		pPlayer->SetPunchAngle( angle );
	}
}

void CWeaponShotgunFD1::SecondaryAttack()
{	
	CSDKPlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		return;

	// don't fire underwater
	if (pPlayer->GetWaterLevel() == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.15;
		return;
	}
		// If my clip is empty (and I use clips) start reload
	if ( UsesClipsForAmmo1() && m_iClip1 < 1 ) 
	{
		PrimaryAttack();
		return;
	}
	
#ifdef GAME_DLL
	pPlayer->NoteWeaponFired();
#endif
	
	pPlayer->DoMuzzleFlash();
	SendWeaponAnim( GetSecondaryAttackActivity() );
	if ( UsesClipsForAmmo1() )
		m_iClip1 -= 2;
	else
		pPlayer->RemoveAmmo(2, m_iPrimaryAmmoType );
	
	pPlayer->IncreaseShotsFired();
	
	float flSpread = GetWeaponSpread() * 1.4; // spead a little bit more than primary
	
	FX_FireBullets(
		pPlayer->entindex(),
		pPlayer->Weapon_ShootPosition(),
		pPlayer->EyeAngles() + pPlayer->GetPunchAngle(),
		GetWeaponID(),
		1, //fire mode. do a double shot (double the bullets released)
		CBaseEntity::GetPredictionRandomSeed() & 255,
		flSpread
		);
		
 
 
	//Add our view kick in
	AddViewKick();
 
	//Tony; update our weapon idle time
	//AM; Currently Sequence Duration is too short ; hack for now.
	//TODO: make model for this weapon so this is appropriate CD. 

	//SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
	SetWeaponIdleTime( gpGlobals->curtime + 1.6f );
 
	//m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
	//m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();
	m_flNextPrimaryAttack = gpGlobals->curtime +  1.6f;
	m_flNextSecondaryAttack = gpGlobals->curtime +  1.6f;
}

bool CWeaponShotgunFD1::Reload()
{
	CSDKPlayer *pPlayer = GetPlayerOwner();

	if (pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 || m_iClip1 == GetMaxClip1())
		return true;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > gpGlobals->curtime)
		return true;
		
	// check to see if we're ready to reload
	if (m_iInSpecialReload == 0)
	{
		pPlayer->SetAnimation( PLAYER_RELOAD );

		SendWeaponAnim( ACT_SHOTGUN_RELOAD_START );
		m_iInSpecialReload = 1;
		pPlayer->m_flNextAttack = gpGlobals->curtime + 0.5;
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.5;
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.5;
		SetWeaponIdleTime( gpGlobals->curtime + 0.5 );
		return true;
	}
	else if (m_iInSpecialReload == 1)
	{
		if (m_flTimeWeaponIdle > gpGlobals->curtime)
			return true;
		// was waiting for gun to move to side
		m_iInSpecialReload = 2;

		SendWeaponAnim( ACT_VM_RELOAD );
		SetWeaponIdleTime( gpGlobals->curtime + 0.45 );
	}
	else if ( m_iInSpecialReload == 2 ) // Sanity, make sure it's actually in the right state.
	{
		// Add them to the clip
		m_iClip1 += 1;
		
#ifdef GAME_DLL
		SendReloadEvents();
#endif
		CSDKPlayer *pPlayer = GetPlayerOwner();

		if ( pPlayer )
			 pPlayer->RemoveAmmo( 1, m_iPrimaryAmmoType );

		m_iInSpecialReload = 1;
	}

	return true;
}


void CWeaponShotgunFD1::WeaponIdle()
{
	CSDKPlayer *pPlayer = GetPlayerOwner();

	if (m_flPumpTime && m_flPumpTime < gpGlobals->curtime)
	{
		// play pumping sound
		m_flPumpTime = 0;
	}

	if (m_flTimeWeaponIdle < gpGlobals->curtime)
	{
		if (m_iClip1 == 0 && m_iInSpecialReload == 0 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ))
		{
			Reload( );
		}
		else if (m_iInSpecialReload != 0)
		{
			if (m_iClip1 != 8 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ))
			{
				Reload( );
			}
			else
			{
				// reload debounce has timed out
				SendWeaponAnim( ACT_SHOTGUN_RELOAD_FINISH );
				
				// play cocking sound
				m_iInSpecialReload = 0;
				SetWeaponIdleTime( gpGlobals->curtime + 1.5 );
				m_flNextPrimaryAttack = gpGlobals->curtime + 0.15; // Add a small delay between finishing reload and firing again
			}
		}
		else
		{
			SendWeaponAnim( ACT_VM_IDLE );
		}
	}
}


//Tony; todo; add ACT_MP_PRONE* activities, so we have them.
acttable_t CWeaponShotgunFD1::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,					ACT_DOD_STAND_IDLE_TOMMY,				false },
	{ ACT_MP_CROUCH_IDLE,					ACT_DOD_CROUCH_IDLE_TOMMY,				false },
	{ ACT_MP_PRONE_IDLE,					ACT_DOD_PRONE_AIM_TOMMY,				false },

	{ ACT_MP_RUN,							ACT_DOD_RUN_AIM_TOMMY,					false },
	{ ACT_MP_WALK,							ACT_DOD_WALK_AIM_TOMMY,					false },
	{ ACT_MP_CROUCHWALK,					ACT_DOD_CROUCHWALK_AIM_TOMMY,			false },
	{ ACT_MP_PRONE_CRAWL,					ACT_DOD_PRONEWALK_IDLE_TOMMY,			false },
	{ ACT_SPRINT,							ACT_DOD_SPRINT_IDLE_TOMMY,				false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_DOD_PRIMARYATTACK_TOMMY,			false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_DOD_PRIMARYATTACK_TOMMY,			false },
	{ ACT_MP_ATTACK_PRONE_PRIMARYFIRE,		ACT_DOD_PRIMARYATTACK_PRONE_TOMMY,		false },

	{ ACT_MP_RELOAD_STAND,					ACT_DOD_RELOAD_TOMMY,					false },
	{ ACT_MP_RELOAD_CROUCH,					ACT_DOD_RELOAD_CROUCH_TOMMY,			false },
	{ ACT_MP_RELOAD_PRONE,					ACT_DOD_RELOAD_PRONE_TOMMY,				false },
};

IMPLEMENT_ACTTABLE( CWeaponShotgunFD1 );
