#ifndef __RCBOT_KEY_VAL__
#define __RCBOT_KEY_VAL__

#define RCBOT_MAX_KV_LEN 128

#include <vector>
using namespace std;

class CRCBotKeyValue
{
public:
	CRCBotKeyValue(char *szKey, char *szValue);

	char *getKey ()
	{
		return m_szKey;
	}

	char *getValue ()
	{
		return m_szValue;
	}

private:
	char m_szKey[RCBOT_MAX_KV_LEN];
	char m_szValue[RCBOT_MAX_KV_LEN];
};

class CRCBotKeyValueList
{
public:
	~CRCBotKeyValueList();

	void parseFile ( FILE *fp );

	//unsigned int size ();

	//CRCBotKeyValue *getKV ( unsigned int iIndex );

	bool getInt ( char *key, int *val );

	bool getString ( char *key, char **val );

	bool getFloat ( char *key, float *val );

private:

	CRCBotKeyValue *getKV ( char *key );

	vector <CRCBotKeyValue*> m_KVs;
};

#endif