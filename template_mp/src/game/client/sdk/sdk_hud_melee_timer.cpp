//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "hud_numericdisplay.h"
#include "iclientmode.h"
#include "iclientvehicle.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui/IVGui.h>
#include "ihudlcd.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;
//-----------------------------------------------------------------------------
// Purpose: Displays current ammunition level
//-----------------------------------------------------------------------------
class CSDKHudMeleeTimer : public CHudNumericDisplay, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CSDKHudMeleeTimer, CHudNumericDisplay );

public:
	CSDKHudMeleeTimer( const char *pElementName );
	virtual void Init( void );
	void VidInit( void );
	virtual void Reset();

	void SetAmmo(int ammo, bool playAnimation);

protected:
	virtual void Paint();
	virtual void OnThink();

	virtual void UpdateAmmoDisplays();
	virtual void UpdatePlayerAmmo( C_BasePlayer *player );
	virtual void ApplySettings(KeyValues *inResourceData);

	wchar_t m_DividerText[32];

private:
	CHandle< C_BaseCombatWeapon > m_hCurrentActiveWeapon;
	CHandle< C_BaseEntity > m_hCurrentVehicle;
	int		m_iAmmo;
	int		m_iTimerValue;
	bool	m_bIsActive;
	int		m_iOrigTall;
	
	int _wide, _tall;
};

DECLARE_HUDELEMENT( CSDKHudMeleeTimer );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSDKHudMeleeTimer::CSDKHudMeleeTimer( const char *pElementName ) : BaseClass(NULL, "HudMeleeTimer"), CHudElement( pElementName )
{
	hudlcd->SetGlobalStat( "(ammo_primary)", "0" );
	hudlcd->SetGlobalStat( "(ammo_secondary)", "0" );
	hudlcd->SetGlobalStat( "(weapon_print_name)", "" );
	hudlcd->SetGlobalStat( "(weapon_name)", "" );
	
	swprintf(m_DividerText, 100, L"%hs", "k");
	
	m_iTimerValue = 0;
	m_iOrigTall = 40;
	m_bIsActive = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSDKHudMeleeTimer::Init( void )
{
	m_iAmmo		= -1;
	
	wchar_t *tempString = g_pVGuiLocalize->Find("#Valve_Hud_AMMO");

	if (tempString)
	{
		SetLabelText(tempString);
	}
	else
	{
		SetLabelText(L"AMMO");
	}
	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSDKHudMeleeTimer::VidInit( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Resets hud after save/restore
//-----------------------------------------------------------------------------
void CSDKHudMeleeTimer::Reset()
{
	BaseClass::Reset();
	
	GetSize(_wide, _tall);

	m_hCurrentActiveWeapon = NULL;
	m_hCurrentVehicle = NULL;
	m_iAmmo = 0;

	UpdateAmmoDisplays();
}

//-----------------------------------------------------------------------------
// Purpose: called every frame to get ammo info from the weapon
//-----------------------------------------------------------------------------
void CSDKHudMeleeTimer::UpdatePlayerAmmo( C_BasePlayer *player )
{
	// Clear out the vehicle entity
	m_hCurrentVehicle = NULL;

	C_BaseCombatWeapon *wpn = GetActiveWeapon();

	hudlcd->SetGlobalStat( "(weapon_print_name)", wpn ? wpn->GetPrintName() : " " );
	hudlcd->SetGlobalStat( "(weapon_name)", wpn ? wpn->GetName() : " " );

	if ( !wpn || !player || wpn->UsesPrimaryAmmo())
	{
		hudlcd->SetGlobalStat( "(ammo_primary)", "n/a" );
		hudlcd->SetGlobalStat( "(ammo_secondary)", "n/a" );

		SetPaintEnabled(false);
		SetPaintBackgroundEnabled(false);
		return;
	}
	
	swprintf(m_DividerText, sizeof(m_DividerText)-1, L"%hc", wpn->GetSpriteActive()->cCharacterInFont );

	m_iTimerValue = min(100, wpn->GetSecondaryAttackTimer());
	m_bIsActive = wpn->GetSecondaryAttackActive();
	//m_iTimerValue = 65;

	SetPaintEnabled(true);
	SetPaintBackgroundEnabled(true);

	// get the ammo in our clip
	int ammo1 = wpn->Clip1();
	int ammo2;
	if (ammo1 < 0)
	{
		// we don't use clip ammo, just use the total ammo count
		ammo1 = player->GetAmmoCount(wpn->GetPrimaryAmmoType());
		ammo2 = 0;
	}
	else
	{
		// we use clip ammo, so the second ammo is the total ammo
		ammo2 = player->GetAmmoCount(wpn->GetPrimaryAmmoType());
	}


	hudlcd->SetGlobalStat( "(ammo_primary)", VarArgs( "%d", ammo1 ) );
	hudlcd->SetGlobalStat( "(ammo_secondary)", VarArgs( "%d", ammo2 ) );
	
	if(wpn){
		
		wchar_t *tempString = new wchar_t[32];
		swprintf(tempString, sizeof(tempString)-1, L"%hc", wpn->GetSpriteAmmo()->cCharacterInFont );
		
		SetLabelText(tempString);
		delete tempString;
	}

	if (wpn == m_hCurrentActiveWeapon)
	{
		// same weapon, just update counts
		SetAmmo(ammo1, true);
	}
	else
	{
		// diferent weapon, change without triggering
		SetAmmo(ammo1, false);

		// update whether or not we show the total ammo display
		if (wpn->UsesClipsForAmmo1())
		{
			SetShouldDisplaySecondaryValue(true);
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("WeaponUsesClips");
		}
		else
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("WeaponDoesNotUseClips");
			SetShouldDisplaySecondaryValue(false);
		}

		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("WeaponChanged");
		m_hCurrentActiveWeapon = wpn;
	}
}

//-----------------------------------------------------------------------------
// Purpose: called every frame to get ammo info from the weapon
//-----------------------------------------------------------------------------
void CSDKHudMeleeTimer::OnThink()
{
	UpdateAmmoDisplays();
}

//-----------------------------------------------------------------------------
// Purpose: updates the ammo display counts
//-----------------------------------------------------------------------------
void CSDKHudMeleeTimer::UpdateAmmoDisplays()
{
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	
	UpdatePlayerAmmo( player );
}

//-----------------------------------------------------------------------------
// Purpose: Updates ammo display
//-----------------------------------------------------------------------------
void CSDKHudMeleeTimer::SetAmmo(int ammo, bool playAnimation)
{
	if (ammo != m_iAmmo)
	{
		if (ammo == 0)
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("AmmoEmpty");
		}
		else if (ammo < m_iAmmo)
		{
			// ammo has decreased
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("AmmoDecreased");
		}
		else
		{
			// ammunition has increased
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("AmmoIncreased");
		}

		m_iAmmo = ammo;
	}

	SetDisplayValue(ammo);
}

void CSDKHudMeleeTimer::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings(inResourceData);
	
	const char *wstr = inResourceData->GetString( "tall", NULL );
	m_iOrigTall = atoi(wstr);

}
//-----------------------------------------------------------------------------
// Purpose: renders the vgui panel
//-----------------------------------------------------------------------------
void CSDKHudMeleeTimer::Paint()
{
	
		//Draw Melee Special Icon
		if(m_bIsActive)
			surface()->DrawSetTextColor(Color(255,255,255,255));
		else surface()->DrawSetTextColor(GetFgColor());
		surface()->DrawSetTextFont(m_hTextFont);
		surface()->DrawSetTextPos(digit_xpos , digit_ypos);
		surface()->DrawUnicodeString( m_DividerText );
		//PaintNumbers(m_hNumberFont, digit_xpos + 30, digit_ypos, m_iTimerValue);
		if((m_iTimerValue/ 100.0) == 1.0) {
			surface()->DrawSetTextColor(GetFgColor());
			surface()->DrawSetTextFont(m_hTextFont);
			surface()->DrawSetTextPos(digit_xpos , digit_ypos);
			surface()->DrawUnicodeString( L"O" );
		}
		
		if(m_bIsActive){
			SetSize(_wide, m_iOrigTall );
		}
		else{
			int newTall= max(1, m_iOrigTall * (m_iTimerValue/ 100.0) );
			SetSize(_wide, newTall );
		}
}