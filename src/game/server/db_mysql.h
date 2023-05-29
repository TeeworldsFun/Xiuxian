#ifdef CONF_SQL
/* SQL Class by Sushi */

#include <engine/shared/protocol.h>

#include <mysql_connection.h>
	
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

#include "Data/accdata.h"

class CSQL
{
public:
	CSQL(class CGameContext *pGameServer);

	sql::Driver *driver;
	sql::Connection *connection;
	sql::Statement *statement;
	sql::ResultSet *results;
	
	// copy of config vars
	const char* database;
	const char* prefix;
	const char* user;
	const char* pass;
	const char* ip;
	int port;
	
	bool connect();
	void disconnect();

	void CreateAccount(SAccData AData);
	void ChangePassword(SAccData AData);

	void Login(SAccData AData);
	void Update(int CID, SAccData AData, const char m_NeedUpdate[256], const char m_Value[256]);
};

struct FaBao
{
    SAccData Data;
    CSQL *m_SqlData;
};

struct FaBao_Update
{
	char m_NeedUpdate[256];
	char m_Value[256];
	SAccData Data;
	CSQL *m_SqlData;
};
#endif