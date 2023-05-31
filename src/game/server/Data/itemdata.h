#ifndef GAME_SERVER_DATA_ITEMDATA_H
#define GAME_SERVER_DATA_ITEMDATA_H

enum
{
    ITEMDATA_DAOLING = 1,      // 道铃
    ITEMDATA_RENDAN_LIEZHI,    // 劣质的人丹
    ITEMDATA_RENDAN_PINGFAN,   // 平凡的人丹
    ITEMDATA_RENDAN_ZHONGDENG, // 中等的人丹
    ITEMDATA_RENDAN_GAODENG, // 高等的人丹
    ITEMDATA_MAJIANG_KONGBAI,  // 空白的麻将牌
    ITEMDATA_QIGUAIDESHIBAN,   // 奇怪的石板
    ITEMDATA_DAQIANLU,         // 大千录

    _______COUNT_ITEM_DATA________,
    NUM_ITEMDATA = _______COUNT_ITEM_DATA________ - 1, //
};

// 喜，怒，悲，贪，嗔，痴，惧，爱，恶
// 生，老，病，死，怨，肉别离，求不得，五蕴盛
enum
{
    ITEMTYPE_ZHUANGBEI = 0,            // 装备
    ITEMTYPE_GONGFA = 1,               // 功法
    ITEMTYPE_DANYAO = 2,               // 丹药
    ITEMTYPE_MIJI = 3,                 // 秘籍
    ITEMTYPE_XIANJIAN = 4,             // 仙剑
    ITEMTYPE_FAZHANG = 5,              // 法杖
    ITEMTYPE_RENWUDAOJU = 6,           // 任务道具
    ITEMTYPE_RENWUDAOJU_KESHIYONG = 7, // 任务道具（可使用）

    NUM_ITEMTYPE,
};

struct SItemDataList
{
    int m_ItemID;
    int m_ItemType;
    char m_Name[512];
    char m_Desc[512];
    int m_Maxium;
};

struct SItemData
{
    int m_UserID;
    int m_ItemID;
    int m_Num;
    char m_Extra[512];
};

#endif