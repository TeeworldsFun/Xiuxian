#ifdef CONF_SQL
/* SQL class by Sushi */
#include "gamecontext.h"
#include "Data/accdata.h"
#include <engine/shared/config.h>

static LOCK sql_lock = 0;
class CGameContext *m_pGameServer;
CGameContext *GameServer() { return m_pGameServer; }

CSQL::CSQL(class CGameContext *pGameServer)
{
    if (sql_lock == 0)
        sql_lock = lock_create();

    m_pGameServer = pGameServer;

    // set database info
    database = g_Config.m_SvSqlDatabase; // 数据库名字
    prefix = "tw";                       // 强制设置为tw防止bug
    user = g_Config.m_SvSqlUser;         // 数据库用户名
    pass = g_Config.m_SvSqlPw;           // 密码
    ip = g_Config.m_SvSqlIp;             // 数据库所在IP
    port = g_Config.m_SvSqlPort;         // 数据库端口
}

bool CSQL::connect()
{
    try
    {
        // Create connection
        driver = get_driver_instance();
        char buf[256];
        str_format(buf, sizeof(buf), "tcp://%s:%d", ip, port);
        connection = driver->connect(buf, user, pass);

        // Create Statement
        statement = connection->createStatement();

        // Create database if not exists
        str_format(buf, sizeof(buf), "CREATE DATABASE IF NOT EXISTS %s", database);
        statement->execute(buf);

        // Connect to specific database
        connection->setSchema(database);
        dbg_msg("SQL", "SQL connection established");
        return true;
    }
    catch (sql::SQLException &e)
    {
        dbg_msg("SQL", "ERROR: SQL connection failed(%s)", e.what());
        return false;
    }
}

void CSQL::disconnect()
{
    try
    {
        delete connection;
        dbg_msg("SQL", "SQL connection disconnected");
    }
    catch (sql::SQLException &e)
    {
        dbg_msg("SQL", "ERROR: No SQL connection");
    }
}

// create Account
static void create_account_thread(void *user)
{
    lock_wait(sql_lock);

    FaBao *Data = (FaBao *)user;

    if (GameServer()->m_apPlayers[Data->Data.m_ClientID])
    {
        // Connect to database
        if (Data->m_SqlData->connect())
        {
            try
            {
                // check if allready exists
                char buf[512];
                str_format(buf, sizeof(buf), "SELECT * FROM %s_Accounts WHERE Username='%s';", Data->m_SqlData->prefix, Data->Data.m_Username);
                Data->m_SqlData->results = Data->m_SqlData->statement->executeQuery(buf);
                if (Data->m_SqlData->results->next())
                {
                    // Account found
                    dbg_msg("SQL", "Account '%s' allready exists", Data->Data.m_Username);

                    GameServer()->SendChatTarget(Data->Data.m_ClientID, _("⚠此方世界容不下两个相同的肉体，还请道友取个别的血肉之名⚠"));
                }
                else
                {
                    // create Account \o/
                    str_format(buf, sizeof(buf), "INSERT INTO %s_Accounts(Username, Password, Language) VALUES ('%s', '%s', '%s');",
                               Data->m_SqlData->prefix,
                               Data->Data.m_Username, Data->Data.m_Password, GameServer()->m_apPlayers[Data->Data.m_ClientID]->GetLanguage());

                    Data->m_SqlData->statement->execute(buf);
                    dbg_msg("SQL", "Account '%s' was successfully created", Data->Data.m_Username);

                    GameServer()->SendChatTarget(Data->Data.m_ClientID, _("⚠⚠⚠⚠⚠⚠⚠⚠ ! ~血⚠肉⚠苦⚠痛~ ! ⚠⚠⚠⚠⚠⚠⚠⚠"));
			        GameServer()->SendChatTarget(Data->Data.m_ClientID, _("血肉之名: {str:Username}"), "Username", Data->Data.m_Username);
			        GameServer()->SendChatTarget(Data->Data.m_ClientID, _("血肉之匙: {str:Password}"), "Password", Data->Data.m_Password);
			        GameServer()->SendChatTarget(Data->Data.m_ClientID, _("使用 /login {str:u} {str:p} 来与你的血肉之体共鸣"), "u", Data->Data.m_Username, "p", Data->Data.m_Password);
			        GameServer()->SendChatTarget(Data->Data.m_ClientID, _("⚠⚠⚠⚠⚠⚠⚠⚠ ! ~魂⚠体⚠飞⚠升~ ! ⚠⚠⚠⚠⚠⚠⚠⚠"));
                }

                // delete statement
                delete Data->m_SqlData->statement;
                delete Data->m_SqlData->results;
            }
            catch (sql::SQLException &e)
            {
                dbg_msg("SQL", "ERROR: Could not create Account(%s)", e.what());
            }

            // disconnect from database
            Data->m_SqlData->disconnect();
        }
    }

    delete Data;

    lock_unlock(sql_lock);
}

void CSQL::CreateAccount(SAccData AData)
{
    FaBao *_FaBao = new FaBao();
    _FaBao->Data = AData;
    _FaBao->m_SqlData = this;

    void *register_thread = thread_init(create_account_thread, _FaBao);
#if defined(CONF_FAMILY_UNIX)
    pthread_detach((pthread_t)register_thread);
#endif
}

// change password
static void change_password_thread(void *user)
{
    lock_wait(sql_lock);

    FaBao *Data = (FaBao *)user;

    // Connect to database
    if (Data->m_SqlData->connect())
    {
        try
        {
            // Connect to database
            Data->m_SqlData->connect();

            // check if Account exists
            char buf[512];
            str_format(buf, sizeof(buf), "SELECT * FROM %s_Accounts WHERE ID='%d';", Data->m_SqlData->prefix, Data->Data.m_UserID);
            Data->m_SqlData->results = Data->m_SqlData->statement->executeQuery(buf);
            if (Data->m_SqlData->results->next())
            {
                // update Account data
                str_format(buf, sizeof(buf), "UPDATE %s_Accounts SET Password='%s' WHERE ID='%d'", Data->m_SqlData->prefix, Data->Data.m_Password, Data->Data.m_UserID);
                Data->m_SqlData->statement->execute(buf);

                // get Account name from database
                str_format(buf, sizeof(buf), "SELECT name FROM %s_Accounts WHERE ID='%d';", Data->m_SqlData->prefix, Data->Data.m_UserID);

                // create results
                Data->m_SqlData->results = Data->m_SqlData->statement->executeQuery(buf);

                // jump to result
                Data->m_SqlData->results->next();

                // finally the name is there \o/
                char acc_name[32];
                str_copy(acc_name, Data->m_SqlData->results->getString("Username").c_str(), sizeof(acc_name));
                dbg_msg("SQL", "Account '%s' changed password.", acc_name);

                // Success
                str_format(buf, sizeof(buf), "Successfully changed your password to '%s'.", Data->Data.m_Password);
                GameServer()->SendBroadcast(buf, Data->Data.m_ClientID);
                GameServer()->SendChatTarget(Data->Data.m_ClientID, buf);
            }
            else
                dbg_msg("SQL", "Account seems to be deleted");

            // delete statement and results
            delete Data->m_SqlData->statement;
            delete Data->m_SqlData->results;
        }
        catch (sql::SQLException &e)
        {
            dbg_msg("SQL", "ERROR: Could not update Account");
        }

        // disconnect from database
        Data->m_SqlData->disconnect();
    }

    delete Data;

    lock_unlock(sql_lock);
}

void CSQL::ChangePassword(SAccData AData)
{
    FaBao *_FaBao = new FaBao();
    _FaBao->Data = AData;
    _FaBao->m_SqlData = this;

    void *change_pw_thread = thread_init(change_password_thread, _FaBao);
#if defined(CONF_FAMILY_UNIX)
    pthread_detach((pthread_t)change_pw_thread);
#endif
}

// login stuff
static void login_thread(void *user)
{
    lock_wait(sql_lock);

    FaBao *Data = (FaBao *)user;

    if (GameServer()->m_apPlayers[Data->Data.m_ClientID] && !GameServer()->m_apPlayers[Data->Data.m_ClientID]->m_AccData.m_UserID)
    {
        // Connect to database
        if (Data->m_SqlData->connect())
        {
            try
            {
                // check if Account exists
                char buf[1024];
                str_format(buf, sizeof(buf), "SELECT * FROM %s_Accounts WHERE Username='%s';", Data->m_SqlData->prefix, Data->Data.m_Username);
                Data->m_SqlData->results = Data->m_SqlData->statement->executeQuery(buf);
                if (Data->m_SqlData->results->next())
                {
                    // check for right pw and get data
                    str_format(buf, sizeof(buf), "SELECT * FROM %s_Accounts WHERE Username='%s' AND Password='%s';",
                               Data->m_SqlData->prefix, Data->Data.m_Username, Data->Data.m_Password);

                    // create results
                    Data->m_SqlData->results = Data->m_SqlData->statement->executeQuery(buf);

                    // if match jump to it
                    if (Data->m_SqlData->results->next())
                    {
                        // never use player directly!
                        // finally save the result to AccountData() \o/

                        // check if Account allready is logged in
                        for (int i = 0; i < MAX_CLIENTS; i++)
                        {
                            if(!GameServer()->m_apPlayers[i])
                                continue;

                            if (GameServer()->m_apPlayers[i]->m_AccData.m_UserID == Data->m_SqlData->results->getInt("ID"))
                            {
                                dbg_msg("SQL", "Account '%s' already is logged in", Data->Data.m_Username);

                                GameServer()->SendChatTarget(Data->Data.m_ClientID, _("⚠ 共鸣失败 ⚠ 此血肉之体已被魂体共鸣⚠"));

                                // delete statement and results
                                delete Data->m_SqlData->statement;
                                delete Data->m_SqlData->results;

                                // disconnect from database
                                Data->m_SqlData->disconnect();

                                // release lock
                                lock_unlock(sql_lock);

                                return;
                            }
                        }

                        // 在这里赋予玩家他账号里的东西
                        GameServer()->m_apPlayers[Data->Data.m_ClientID]->m_AccData.m_UserID = Data->m_SqlData->results->getInt("ID");
                        GameServer()->m_apPlayers[Data->Data.m_ClientID]->m_AccData.m_Xiuwei = Data->m_SqlData->results->getInt("Xiuwei");
                        GameServer()->m_apPlayers[Data->Data.m_ClientID]->m_AccData.m_Po = Data->m_SqlData->results->getInt("Po");

                        // login should be the last thing
                        dbg_msg("SQL", "Account '%s' logged in sucessfully", Data->Data.m_Username);

                        GameServer()->SendChatTarget(Data->Data.m_ClientID, _("⚠ 共鸣成功 ⚠ 欢迎来到诡异界 ⚠"));
                    }
                    else
                    {
                        // wrong password
                        dbg_msg("SQL", "Account '%s' is not logged in due to wrong password", Data->Data.m_Username);

                        GameServer()->SendChatTarget(Data->Data.m_ClientID, _("⚠ 共鸣失败 ⚠ 此方世界没有找到对应血肉体 ⚠"));
                    }
                }
                else
                {
                    // no Account
                    dbg_msg("SQL", "Account '%s' does not exists", Data->Data.m_Username);

                    GameServer()->SendChatTarget(Data->Data.m_ClientID, _("⚠ 共鸣失败 ⚠ 此方世界没有找到对应血肉体 ⚠"));
                }

                // delete statement and results
                delete Data->m_SqlData->statement;
                delete Data->m_SqlData->results;
            }
            catch (sql::SQLException &e)
            {
                dbg_msg("SQL", "ERROR: Could not login Account(%s)", e.what());
            }

            // disconnect from database
            Data->m_SqlData->disconnect();
        }
    }

    delete Data;

    lock_unlock(sql_lock);
}

void CSQL::Login(SAccData AData)
{
    FaBao *_FaBao = new FaBao();
    _FaBao->Data = AData;
    _FaBao->m_SqlData = this;

    void *login_account_thread = thread_init(login_thread, _FaBao);
#if defined(CONF_FAMILY_UNIX)
    pthread_detach((pthread_t)login_account_thread);
#endif
}

// update stuff
static void update_thread(void *user)
{
    lock_wait(sql_lock);

    FaBao_Update *Data = (FaBao_Update *)user;

    // Connect to database
    if (Data->m_SqlData->connect())
    {
        try
        {
            // check if Account exists
            char buf[1024];
            str_format(buf, sizeof(buf), "SELECT * FROM %s_Accounts WHERE ID='%d';", Data->m_SqlData->prefix, Data->Data.m_UserID);
            Data->m_SqlData->results = Data->m_SqlData->statement->executeQuery(buf);
            if (Data->m_SqlData->results->next())
            {
                // update Account data
                str_format(buf, sizeof(buf), "UPDATE %s_Accounts SET "
                                             "%s='%s' "
                                             "WHERE ID='%d';",
                           Data->m_SqlData->prefix, Data->m_NeedUpdate, Data->m_Value, Data->Data.m_UserID);
                Data->m_SqlData->statement->execute(buf);

                str_append(buf, " WHERE UserID = %d;", Data->Data.m_UserID);

                // get Account name from database
                str_format(buf, sizeof(buf), "SELECT Username FROM %s_Accounts WHERE ID='%d';", Data->m_SqlData->prefix, Data->Data.m_UserID);

                // create results
                Data->m_SqlData->results = Data->m_SqlData->statement->executeQuery(buf);

                // jump to result
                Data->m_SqlData->results->next();

                // finally the nae is there \o/
                char acc_name[32];
                str_copy(acc_name, Data->m_SqlData->results->getString("Username").c_str(), sizeof(acc_name));
                dbg_msg("SQL", "Account '%s' was saved successfully", acc_name);
            }
            else
                dbg_msg("SQL", "Account seems to be deleted");

            // delete statement and results
            delete Data->m_SqlData->statement;
            delete Data->m_SqlData->results;
        }
        catch (sql::SQLException &e)
        {
            dbg_msg("SQL", "ERROR: Could not update Account");
        }

        // disconnect from database
        Data->m_SqlData->disconnect();
    }

    delete Data;

    lock_unlock(sql_lock);
}

void CSQL::Update(int CID, SAccData AData, const char m_NeedUpdate[256], const char m_Value[256])
{
    FaBao_Update *_FaBao = new FaBao_Update();
    _FaBao->Data = AData;
    _FaBao->m_SqlData = this;
    str_format(_FaBao->m_NeedUpdate, sizeof(_FaBao->m_NeedUpdate), m_NeedUpdate);
    str_format(_FaBao->m_Value, sizeof(_FaBao->m_Value), m_Value);

    void *update_account_thread = thread_init(update_thread, _FaBao);
#if defined(CONF_FAMILY_UNIX)
    pthread_detach((pthread_t)update_account_thread);
#endif
}

#endif