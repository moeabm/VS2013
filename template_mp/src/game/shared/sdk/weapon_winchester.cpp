//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_sdkbase.h"
#include "sdk_fx_shared.h"


#if defined( CLIENT_DLL )

	#define CWeaponWinchester C_WeaponWinchester
	#include "c_sdk_player.h"

#else

	#include "sdk_player.h"
	#include "te_firebullets.h"

#endif


class CWeaponWinchester : public CWeaponSDKBase
{
public:
	DECLARE_CLASS( CWeaponWinchester, CWeaponSDKBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponWinchester();

	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual bool Reload();
	virtual void WeaponIdle();
	virtual bool ReloadOrSwitchWeapons();
	virtual bool Holster(CBaseCombatWeapon *switchingto);

	virtual SDKWeaponID GetWeaponID( void ) const		{ return SDK_WEAPON_WINCHESTER; }
	virtual float GetWeaponSpread() { return 0.005f; } // AM; this weapon is VERY accurate


private:

	CWeaponWinchester( const CWeaponWinchester & );

	float m_flPumpTime;
	CNetworkVar( int, m_iInSpecialReload );

};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponWinchester, DT_WeaponWinchester )

BEGIN_NETWORK_TABLE( CWeaponWinchester, DT_WeaponWinchester )

	#ifdef CLIENT_DLL
		RecvPropInt( RECVINFO( m_iInSpecialReload ) )
	#else
		SendPropInt( SENDINFO( m_iInSpecialReload ), 2, SPROP_UNSIGNED )
	#endif

END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponWinchester )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_iInSpecialReload, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_winchester, CWeaponWinchester );
PRECACHE_WEAPON_REGISTER( weapon_winchester );



CWeaponWinchester::CWeaponWinchester()
{
	m_flPumpTime = 0;
	m_iClip2 = 1;
}

void CWeaponWinchester::PrimaryAttack()
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

void CWeaponWinchester::SecondaryAttack()
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

	//Zoom FOV 
	if(pPlayer->GetFOV() < 90) pPlayer->SetFOV(pPlayer,90, 0.5f);
	else pPlayer->SetFOV(pPlayer,40, 0.5f);

	
	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
 
	//m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	m_flNextSecondaryAttack = gpGlobals->curtime + GetFireRate();
}

bool CWeaponWinchester::Reload()
{
	CSDKPlayer *pPlayer = GetPlayerOwner();

	if (pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 || (m_iClip1 == GetMaxClip1() && m_iClip2 == GetMaxClip2()))
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
		bool priReloaded = false;
		bool secReloaded = false;
		if(m_iClip1 < GetMaxClip1()){
			m_iClip1 += 1;
			priReloaded = true;
		}
		if(m_iClip2 < GetMaxClip2() && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) > 2){
			m_iClip2 += 1;
			secReloaded = true;
		}
		
#ifdef GAME_DLL
		SendReloadEvents();
#endif
		CSDKPlayer *pPlayer = GetPlayerOwner();

		if ( pPlayer ) {
			if(priReloaded){
			   pPlayer->RemoveAmmo( 1, m_iPrimaryAmmoType );
			}
			if(secReloaded){
			   pPlayer->RemoveAmmo( 1, m_iPrimaryAmmoType );
			}
		}

		m_iInSpecialReload = 1;
	}

	return true;
}


void CWeaponWinchester::WeaponIdle()
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
			//commented out : dont auto reload @ zero bullets in clip
			//Reload( );
		}
		else if (m_iInSpecialReload != 0)
		{
			if ((m_iClip1 < GetMaxClip1() || m_iClip2 < GetMaxClip2()) && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ))
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
bool CWeaponWinchester::ReloadOrSwitchWeapons( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	Assert( pOwner );

	m_bFireOnEmpty = false;

	// If we don't have any ammo, switch to the next best weapon
	if ( !HasAnyAmmo() && m_flNextPrimaryAttack < gpGlobals->curtime && m_flNextSecondaryAttack < gpGlobals->curtime )
	{
		// weapon isn't useable, switch.
		if ( ( (GetWeaponFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY) == false ) && ( g_pGameRules->SwitchToNextBestWeapon( pOwner, this ) ) )
		{
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.3;
			return true;
		}
	}
	else
	{
		// Weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
		if ( UsesClipsForAmmo1() && !AutoFiresFullClip() && 
			 (m_iClip1 == 0) && (m_iClip2 == 0) && 
			 (GetWeaponFlags() & ITEM_FLAG_NOAUTORELOAD) == false && 
			 m_flNextPrimaryAttack < gpGlobals->curtime && 
			 m_flNextSecondaryAttack < gpGlobals->curtime )
		{
			// if we're successfully reloading, we're done
			if ( Reload() )
				return true;
		}
	}

	return false;
}

bool CWeaponWinchester::Holster( CBaseCombatWeapon *pSwitchingTo )
{ 
	GetPlayerOwner()->SetFOV(GetPlayerOwner(),90, 0.1f);
	return BaseClass::Holster(pSwitchingTo);
}

//Tony; todo; add ACT_MP_PRONE* activities, so we have them.
acttable_t CWeaponWinchester::m_acttable[] = 
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

IMPLEMENT_ACTTABLE( CWeaponWinchester );
