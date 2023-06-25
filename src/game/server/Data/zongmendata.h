#ifndef GAME_SERVER_CLASSESDATA_H
#define GAME_SERVER_CLASSESDATA_H

enum
{
    ZONGMEN_SANXIU = 0,   // 散修
    ZONGMEN_XIESUI,       // 邪祟阵营
    ZONGMEN_AOJINGJIAO,   // 袄景教
    ZONGMEN_BAILIANJIAO,  // 白莲教
    ZONGMEN_ZHENGDESI,    // 正德寺
    ZONGMEN_ANCIAN,       // 安慈庵
    ZONGMEN_CHUMAXIAN,    // 出马仙
    ZONGMEN_NUOXI,        // 傩戏
    ZONGMEN_BINGJIA,      // 兵家
    ZONGMEN_WUSHIGONG,    // 舞狮宫
    ZONGMEN_LUOJIA,       // 罗教
    ZONGMEN_ZHONGYINMIAO, // 中阴庙
    ZONGMEN_FAJIAO,       // 法教
    ZONGMEN_FANGXIANDAO,  // 方仙道
    ZONGMEN_QINGFENGGUAN, // 清风观
    ZONGMEN_ZUOWANGDAO,   // 何谓坐忘道? 堕其肢体，黜遁聪者，离形去知，同于大通，此谓坐忘。

    __COUNT__NUM___________ZONGMEN___COUNT__,
    NUM_ZONGMEN = __COUNT__NUM___________ZONGMEN___COUNT__ - 1,
};

struct SZongMenData
{
    int m_ID;
    char m_Name[64];
    char m_Desc[512];
    int m_ZongZhu;
};

#endif