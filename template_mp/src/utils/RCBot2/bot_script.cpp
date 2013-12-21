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
#include <string.h>

#include "bot.h"
#include "bot_script.h"
#include "bot_fortress.h"
#include "bot_strings.h"
#include "bot_globals.h"

vector <CResetPoint*> CPoints::m_points;

vector <int> CPoints::m_BlueAttack; 
vector <int> CPoints::m_RedAttack; 
vector <int> CPoints::m_BlueDefend; 
vector <int> CPoints::m_RedDefend;
int CPoints::m_iValidAreas = 0;
float CPoints::m_fLoadedScript = 0.0f;

CPointStyle::CPointStyle (int iPointArea,ePointStyle iStyle)
{
		m_iPointArea = iPointArea;
		m_iStyle = iStyle;
}

void CResetPoint :: getCurrentPoints ( int iTeamCaptured, int iTeam, vector<CPointStyle> **iNextPoints )
{
	*iNextPoints = NULL;

// when blue captures (or is beginning)
	if ( isResetPoint() || (iTeamCaptured == TF2_TEAM_BLUE) )
	{
		if ( iTeam == TF2_TEAM_BLUE )
			*iNextPoints = &m_iNextBlue[0];
		else if ( iTeam == TF2_TEAM_RED )
			*iNextPoints = &m_iNextRed[0];
	}
	// When red captured
	else
	{
		if ( iTeam == TF2_TEAM_BLUE )
			*iNextPoints = &m_iNextBlue[1];
		else if ( iTeam == TF2_TEAM_RED )
			*iNextPoints = &m_iNextRed[1];
	}
}

void CResetPoint :: addPointStyle ( int iTeamCaptured, int iTeam, CPointStyle pointstyle )
{
	if ( isResetPoint() || (iTeamCaptured == TF2_TEAM_BLUE) )
	{
		if ( iTeam == TF2_TEAM_BLUE )
			m_iNextBlue[0].push_back(pointstyle);
		else
			m_iNextRed[0].push_back(pointstyle);
	}
	else if ( iTeamCaptured == TF2_TEAM_RED )
	{
		if ( iTeam == TF2_TEAM_BLUE )
			m_iNextBlue[1].push_back(pointstyle);
		else
			m_iNextRed[1].push_back(pointstyle);
	}
}

void CResetPoint :: addValidArea ( int iArea )
{
	m_iValidAreas.push_back(iArea);
}

CPoint :: CPoint ( const char *szName )
{
	m_szName = CStrings::getString(szName);
}

bool CPoint :: isPoint ( const char *szName ) 
{ 
	return !strcmp(szName,m_szName); 
}

float CPoints::getPointCaptureTime ( unsigned int id )
{
	if ( id < m_points.size() )
		return m_points[id]->getCaptureTime();

	return 0.0f;
}

CResetPoint *CPoints::getPoint ( const char *szName )
{
	unsigned int i;
	CResetPoint *newpoint = NULL;

    for ( i = 0; i < m_points.size(); i ++ )
	{
		if ( szName == NULL )
		{
			if ( m_points[i]->isResetPoint() )
				return m_points[i];
		}
		else if ( m_points[i]->isPoint(szName) )
			return m_points[i];
	}

	if ( szName )
		newpoint = new CPoint(szName);
	else
		newpoint = new CResetPoint();

	m_points.push_back(newpoint);

	return newpoint;
}

int CResetPoint :: getValidAreas ( void )
{
	unsigned int i;

	int iAreas = 0;

	for ( i = 0; i < m_iValidAreas.size(); i ++ )
	{
		iAreas |= (1<<(m_iValidAreas[i]));
	}

	return iAreas;
}

void CResetPoint :: updateCaptureTime ()
{
	m_fCaptureTime = engine->Time();
}

void CPoints :: pointBeingCaptured ( int iTeam, const char *szName )
{
	CResetPoint *p = CPoints::getPoint(szName);

	if ( p )
		p->updateCaptureTime();
}

void CPoints :: pointCaptured ( int iTeamCaptured, const char *szName )
{
	CResetPoint *p = CPoints::getPoint(szName);

	m_BlueAttack.clear(); 
	m_RedAttack.clear(); 
	m_BlueDefend.clear(); 
	m_RedDefend.clear(); 
	m_iValidAreas = 0;
	//m_ValidAreas.clear();

	if ( p )
	{
		vector<CPointStyle> *points;

		m_iValidAreas = p->getValidAreas();

		p->getCurrentPoints(iTeamCaptured,TF2_TEAM_BLUE,&points);

		if ( points )
		{
			unsigned int i = 0; 
			
			for ( i = 0; i < points->size(); i ++ )
			{
				if ( (*points)[i].getStyle () == POINT_DEFEND )
					m_BlueDefend.push_back((*points)[i].getArea());
				else if ( (*points)[i].getStyle () == POINT_ATTACK )
					m_BlueAttack.push_back((*points)[i].getArea());
			}
		}

		p->getCurrentPoints(iTeamCaptured,TF2_TEAM_RED,&points);

		if ( points )
		{
			unsigned int i = 0; 
			
			for ( i = 0; i < points->size(); i ++ )
			{
				if ( (*points)[i].getStyle () == POINT_DEFEND )
					m_RedDefend.push_back((*points)[i].getArea());
				else if ( (*points)[i].getStyle () == POINT_ATTACK )
					m_RedAttack.push_back((*points)[i].getArea());
			}
		}
	}
}

void CPoints :: resetPoints()
{
	CResetPoint *p = CPoints::getPoint(NULL);

	m_BlueAttack.clear(); 
	m_RedAttack.clear(); 
	m_BlueDefend.clear(); 
	m_RedDefend.clear(); 
	m_iValidAreas = 0;
	//m_ValidAreas.clear();
	m_fLoadedScript = 0.0f;

	if ( p )
	{
		vector<CPointStyle> *points;

		m_iValidAreas = p->getValidAreas();

		p->getCurrentPoints(0,TF2_TEAM_BLUE,&points);

		if ( points )
		{
			unsigned int i = 0; 
			
			for ( i = 0; i < points->size(); i ++ )
			{
				if ( (*points)[i].getStyle () == POINT_DEFEND )
					m_BlueDefend.push_back((*points)[i].getArea());
				else if ( (*points)[i].getStyle () == POINT_ATTACK )
					m_BlueAttack.push_back((*points)[i].getArea());
			}
		}

		p->getCurrentPoints(0,TF2_TEAM_RED,&points);

		if ( points )
		{
			unsigned int i = 0; 
			
			for ( i = 0; i < points->size(); i ++ )
			{
				if ( (*points)[i].getStyle () == POINT_DEFEND )
					m_RedDefend.push_back((*points)[i].getArea());
				else if ( (*points)[i].getStyle () == POINT_ATTACK )
					m_RedAttack.push_back((*points)[i].getArea());
			}
		}
	}
}

// TODO : convert to [vector] input
void CPoints :: getAreas( int iTeam, int *iDefend, int *iAttack )
{
	if ( m_points.size() )
	{
		if ( iTeam == TF2_TEAM_BLUE )
		{
			if ( m_BlueAttack.size() )
				*iAttack = m_BlueAttack[0];
			if ( m_BlueDefend.size() )
				*iDefend = m_BlueDefend[0];
		}
		else if ( iTeam == TF2_TEAM_RED )
		{
			if ( m_RedAttack.size() )
				*iAttack = m_RedAttack[0];
			if ( m_RedDefend.size() )
				*iDefend = m_RedDefend[0];
		}
	}
}

void CPoints :: freeMemory ()
{
	unsigned int i = 0;

	m_BlueAttack.clear(); 
	m_RedAttack.clear(); 
	m_BlueDefend.clear(); 
	m_RedDefend.clear(); 
	//m_ValidAreas.clear();

	for ( i = 0; i < m_points.size(); i ++ )
	{
		delete m_points[i];
		m_points[i] = NULL;
	}

	m_points.clear();
}

void CPoints :: loadMapScript ( )
{
	char filename[1024];

	vector < CResetPoint* > points;
	CResetPoint *currentpoint = NULL;

	//if ( m_fLoadedScript > engine->Time() )
	//	return;

	//m_fLoadedScript = engine->Time() + 10.0f;

	freeMemory();
	
	//CBotGlobals::buildFileName(filename,CBotGlobals::getMapName(),BOT_SCRIPT_FOLDER,BOT_SCRIPT_EXTENSION);

	CBotGlobals::buildFileName(filename,CBotGlobals::getMapName(),BOT_WAYPOINT_FOLDER,BOT_SCRIPT_EXTENSION,true);

	FILE *fp = CBotGlobals::openFile(filename,"r");

	int state = STATE_NONE;

	if ( fp )
	{
		char line[256];
		int len;

		int linenum = 0;

		while ( fgets(line,255,fp) != NULL )
		{
			line[255] = 0;

			linenum++;

			if ( line[0] == '#' )
				continue;

			len = strlen(line);

            if ( line[len-1] == '\n' )
                line[--len] = 0;
#ifdef __linux__
            if ( line[len-1] == '\r' )    
                line[--len] = 0;
#endif
            if ( len < 1 )
                continue;

			// control character
			if ( line[0] == ':' )
			{
				if ( strncmp(&line[1],"arena_point_time:",17) == 0 )
				{
					CTeamFortress2Mod::setPointOpenTime(atoi(&line[18]));
				}
				else if ( strncmp(&line[1],"attack_defend_map:",18) == 0 )
				{
					CTeamFortress2Mod::setAttackDefendMap(line[19]=='1');
				}
				else if ( strncmp(&line[1],"setup_time:",11) == 0 )
				{
					CTeamFortress2Mod::setSetupTime(atoi(&line[12]));
				}
				else if ( strncmp(&line[1],"on_blue_cap:",12) == 0 )
				{
					currentpoint = getPoint(&line[13]);
					state = STATE_BLUE_CAP;
				}
				else if ( strncmp(&line[1],"on_red_cap:",11) == 0 )
				{
					currentpoint = getPoint(&line[12]);
					state = STATE_RED_CAP;
				}
				else if ( strncmp(&line[1],"on_reset:",9) == 0 )
				{
					currentpoint = getPoint(NULL);
					state = STATE_RESET;
				}
			}
			else if ( state != STATE_NONE )
			{
				int iTeam = 0;
				ePointStyle style;
			
				int i,n;
				char num[4];

				num[0] = 0;

				if ( strncmp("areas:",&line[0],6) == 0 )
				{
					// get areas
					i = 6;
					n = 0;

					while ( i < len )
					{
						if (line[i] == ' ')
						{
							i++;
							continue;
						}

						if ( n < 3 )
							num[n++] = line[i];

						if ( ((i+1) >= len) || (line[i+1]==',') )
						{
							num[n]=0;
							n = 0;

							currentpoint->addValidArea(atoi(num));

							i++;//skip ,
						}

						i++;
					}

					continue;
				}
				else if ( strncmp("red_",&line[0],4) == 0 )
				{
					iTeam = TF2_TEAM_RED;
					i = 4;
				}
				else if ( strncmp("blue_",&line[0],5) == 0 )
				{
					iTeam = TF2_TEAM_BLUE;
					i = 5;
				}
				else
				{
					CBotGlobals::botMessage(NULL,0,"SCRIPT Syntax Error line : %d, missing team name",linenum);
					// Syntax Error : missing team name
					continue;
				}

				if ( strncmp("attack:",&line[i],7) == 0) 
					style = POINT_ATTACK;
				else if ( strncmp("defend:",&line[i],7) == 0 )
					style = POINT_DEFEND;
				else
				{
					CBotGlobals::botMessage(NULL,0,"SCRIPT Syntax Error line : %d, missing attack/defend",linenum);
					// Syntax Error : missing attack/defend
					continue;
				}

				// get points

				i = i + 7;
				n = 0;

				while ( i < len )
				{
					if (line[i] == ' ')
					{
						i++;
						continue;
					}

					if ( n < 3 )
						num[n++] = line[i];

					if ( ((i+1) >= len) || (line[i+1]==',') )
					{
						num[n]=0;
						n = 0;

						if ( state == STATE_RESET )
							currentpoint->addPointStyle(0,iTeam,CPointStyle(atoi(num),style));
						else if ( state == STATE_BLUE_CAP )
							currentpoint->addPointStyle(TF2_TEAM_BLUE,iTeam,CPointStyle(atoi(num),style));
						else if ( state == STATE_RED_CAP )
							currentpoint->addPointStyle(TF2_TEAM_RED,iTeam,CPointStyle(atoi(num),style));

						i++;//skip ,
					}

					i++;
				}
				
			}
		}
		fclose(fp);
	}
	else
		return;
}