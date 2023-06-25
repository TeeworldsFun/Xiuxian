#pragma once

enum
{
    ITEMDATA_DAOLING = 1,      // 道铃
    ITEMDATA_RENDAN_LIEZHI,    // 劣质的人丹
    ITEMDATA_RENDAN_PINGFAN,   // 平凡的人丹
    ITEMDATA_RENDAN_ZHONGDENG, // 中等的人丹
    ITEMDATA_RENDAN_GAODENG,   // 高等的人丹
    ITEMDATA_MAJIANG_KONGBAI,  // 空白的麻将牌
    ITEMDATA_QIGUAIDESHIBAN,   // 奇怪的石板
    ITEMDATA_DAQIANLU,         // 大千录
    ITEMDATA_TONGQIANMIANZHAO, // 铜钱面罩
    ITEMDATA_HUOAOZHENJING,    // 火袄真经

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

struct SItemData_Function
{
    const char *GetItemTypeName(int ID)
    {
        switch (ID)
        {
        case ITEMTYPE_ZHUANGBEI:
            return "装备";
        case ITEMTYPE_GONGFA:
            return "功法";
        case ITEMTYPE_DANYAO:
            return "丹药";
        case ITEMTYPE_MIJI:
            return "秘籍";
        case ITEMTYPE_XIANJIAN:
            return "仙剑";
        case ITEMTYPE_FAZHANG:
            return "法杖";
        case ITEMTYPE_RENWUDAOJU:
            return "任务道具";
        case ITEMTYPE_RENWUDAOJU_KESHIYONG:
            return "任务道具(可使用)";
        default:
            return "李兄，救我，我好痛苦！"; // 指遇到了Bug很难受
            break;
        }
    }
    int GetItemType(int ID)
    {
        switch (ID)
        {
        case ITEMDATA_DAOLING:
        case ITEMDATA_TONGQIANMIANZHAO:
        case ITEMDATA_QIGUAIDESHIBAN:
            return ITEMTYPE_ZHUANGBEI;

        case ITEMDATA_RENDAN_LIEZHI:
        case ITEMDATA_RENDAN_PINGFAN:
        case ITEMDATA_RENDAN_ZHONGDENG:
        case ITEMDATA_RENDAN_GAODENG:
            return ITEMTYPE_DANYAO;

        case ITEMDATA_MAJIANG_KONGBAI:
            return ITEMTYPE_RENWUDAOJU;

        case ITEMDATA_DAQIANLU:
        case ITEMDATA_HUOAOZHENJING:
            return ITEMTYPE_GONGFA;

        default:
            return 114514; // 指遇到了Bug很难受
            break;
        }
    }
};