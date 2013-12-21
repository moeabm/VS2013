/*
 *    This file is part of RCBot.
 *
 *    RCBot by Paul Murphy adapted from Botman's HPB Bot 2 template.
 *
 *    RCBot is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    RCBot is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with RCBot; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */
#ifndef __BOT_SCRIPT_H__
#define __BOT_SCRIPT_H__


#include <vector>
using namespace std;

typedef enum 
{
	POINT_NONE,
	POINT_DEFEND,
	POINT_ATTACK
}ePointStyle;


#define STATE_NONE 0
#define STATE_RESET 1
#define STATE_BLUE_CAP 2
#define STATE_RED_CAP 3
	
class CPointStyle
{
public:
	CPointStyle (int iPointArea,ePointStyle iStyle);

	inline int getArea () { return m_iPointArea; }
	inline ePointStyle getStyle () { return m_iStyle; }
private:
	int m_iPointArea;
	ePointStyle m_iStyle;
};

class CResetPoint
{
public:
	void getCurrentPoints ( int iTeamCaptured, int iTeam, vector<CPointStyle> **iNextPoints);

	virtual bool isResetPoint () { return true; }
	virtual bool isPoint ( const char *szName ) { return false; }

	void addPointStyle ( int iTeamCaptured, int iTeam, CPointStyle pointstyle );

	void addValidArea ( int iArea );

	int getValidAreas ( void );

	void updateCaptureTime ();

	inline float getCaptureTime () { return m_fCaptureTime; }

private:
	vector <CPointStyle> m_iNextRed[2];
	vector <CPointStyle> m_iNextBlue[2];
	vector <int> m_iValidAreas;
	float m_fCaptureTime;
};

class CPoint : public CResetPoint
{
public:
	CPoint ( const char *szName );

	bool isResetPoint () { return false; }
	bool isPoint ( const char *szName ); 
private:
	const char *m_szName;
};

class CPoints
{
public:
	
	static void pointCaptured ( int iTeamCaptured, const char *szName );

	static void pointBeingCaptured ( int iTeam, const char *szName );

	static float getPointCaptureTime ( unsigned int id );

	static void resetPoints();

	static void loadMapScript ();

	// TODO : convert to [vector]
	static void getAreas ( int iTeam, int *iDefend, int *iAttack );

	static void freeMemory ();

	static inline int getValidAreas () { return m_iValidAreas; }

	static inline bool isValidArea ( int iArea ) { return (iArea==0)||(m_iValidAreas==0) || ((m_iValidAreas & (1<<iArea))>0); }
private:

	static CResetPoint *getPoint ( const char *szName = NULL );

	static vector <CResetPoint*> m_points;

	static vector <int> m_BlueAttack; 
	static vector <int> m_RedAttack; 
	static vector <int> m_BlueDefend; 
	static vector <int> m_RedDefend; 
	static int m_iValidAreas;
	static float m_fLoadedScript;
};


#endif