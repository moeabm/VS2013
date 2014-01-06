//======== Copyright � 1996-2008, Valve Corporation, All rights reserved. =========//
//
// Purpose: 
//
// $NoKeywords: $
//=================================================================================//

#ifndef SDK_TEAMMENU_H
#define SDK_TEAMMENU_H

#include "teammenu.h"
#include <vgui_controls/EditablePanel.h>
#include "iconpanel.h"

#if defined ( SDK_USE_TEAMS )
class CSDKTeamInfoPanel : public vgui::EditablePanel
{
private:
	DECLARE_CLASS_SIMPLE( CSDKTeamInfoPanel, vgui::EditablePanel );

public:
	CSDKTeamInfoPanel( vgui::Panel *parent, const char *panelName ) : vgui::EditablePanel( parent, panelName )
	{
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual vgui::Panel *CreateControlByName( const char *controlName );
};

class CSDKTeamMenu : public CTeamMenu
{
private:
	DECLARE_CLASS_SIMPLE( CSDKTeamMenu, CTeamMenu );
	// VGUI2 override
	void OnCommand( const char *command);
	// helper functions
	void SetVisibleButton(const char *textEntryName, bool state);

public:
	CSDKTeamMenu(IViewPort *pViewPort);
	virtual ~CSDKTeamMenu();

	void Update();
	virtual void SetVisible(bool state);
	void MoveToCenterOfScreen();
	virtual Panel *CreateControlByName( const char *controlName );
	virtual void ShowPanel(bool bShow);

protected:
	// vgui overrides for rounded corner background
	virtual void PaintBackground();
	virtual void PaintBorder();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

private:
	// rounded corners
	Color					 m_bgColor;
	Color					 m_borderColor;
	CSDKTeamInfoPanel *m_pTeamInfoPanel;
	MouseOverButton<CSDKTeamInfoPanel> *m_pInitialButton;

};
#endif // SDK_USE_TEAMS

#endif //SDK_CLASSMENU_H