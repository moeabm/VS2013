//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//
// Health.cpp
//
// implementation of CHudTimer class
//
#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "view.h"

#include "iclientmode.h"

#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>

#include <vgui/ILocalize.h>

using namespace vgui;

#include "hudelement.h"
#include "hud_basetimer.h"
#include "sdk_gamerules.h"

#include "ConVar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Health panel
//-----------------------------------------------------------------------------
class CHudTimer : public CHudElement, public CHudBaseTimer
{
	DECLARE_CLASS_SIMPLE( CHudTimer, CHudBaseTimer );

public:
	CHudTimer( const char *pElementName );
	virtual void Init();
	virtual void Reset();

protected:
	virtual void Paint();

private:
};	

DECLARE_HUDELEMENT( CHudTimer );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudTimer::CHudTimer( const char *pElementName ) : CHudElement( pElementName ), CHudBaseTimer(NULL, "HudTimer")
{
	SetHiddenBits( HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTimer::Init()
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTimer::Reset()
{
}


//-----------------------------------------------------------------------------
// Purpose: renders the vgui panel
//-----------------------------------------------------------------------------
void CHudTimer::Paint()
{
	int totalSecs = SDKGameRules()->GetRoundTimerRemaining();
	totalSecs = MAX(totalSecs, 0);
	SetMinutes( totalSecs / 60);
	SetSeconds( totalSecs % 60);
	BaseClass::Paint();
}