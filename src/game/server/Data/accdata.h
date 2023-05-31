#ifndef GAME_SERVER_ACCDATA_H
#define GAME_SERVER_ACCDATA_H

#include <vector>
#include "itemdata.h"

static const int s_YuansuNum = 5;

struct SAccData
{
	int m_ClientID; // 玩家灵魂（启动器）身份证，不存入数据库
	int m_UserID;	// 用户身份证(什)

	char m_Username[32]; // 用户名
	char m_Password[32]; // 密码

	int m_Xiuwei; // 修为
	int m_Po;	  // 魄
	int m_Tizhi;  // 体质

	int m_YuanSu[s_YuansuNum]; // 元素 保存形式csv

	std::vector<SItemData> m_vItemData;
};

struct AccDataList
{
	enum
	{
		ACCDATA_CLIENTID = -1, // 不收录进数据库
		ACCDATA_USERID, // 用户ID

		ACCDATA_USERNAME, // 用户名
		ACCDATA_PASSWORD, // 密码

		ACCDATA_XIUWEI, // 修为
		ACCDATA_PO, // 魄
		ACCDATA_TIZHI, // 体质

		ACCDATA_YUANSU, // 元素

		NUM_ACCDATA, // 数量
	};

	const char *GetDataName(int ID) // 返回对应数据ID在数据库里的名字
	{
		switch (ID)
		{
		case ACCDATA_USERID:	return "UserID";
		case ACCDATA_USERNAME:  return "Username";
		case ACCDATA_PASSWORD:  return "Password";
		case ACCDATA_XIUWEI:    return "Xiuwei";
		case ACCDATA_PO:        return "Po";
		case ACCDATA_TIZHI:     return "Tizhi";
		case ACCDATA_YUANSU:    return "Yuansu";
		}
	}

	const char *GetDatabaseName(int ID)
	{
		switch (ID)
		{
		case ACCDATA_USERID:
		case ACCDATA_XIUWEI:
		case ACCDATA_PO:    
		case ACCDATA_TIZHI: 
		case ACCDATA_YUANSU:
			return "tw_Accounts";
		
		default:
			break;
		}
	}
};

#endif