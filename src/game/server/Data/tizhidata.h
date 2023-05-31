#ifndef GAME_SERVER_TIZHIDATA_H
#define GAME_SERVER_TIZHIDATA_H

#include <cmath>

enum
{
    TIZHI_NONE = 0,         // 没有特殊体质
    TIZHI_XINSU = 1 << 0,   // 心素
    TIZHI_XINZHUO = 1 << 1, // 心浊
    TIZHI_XINPAN = 1 << 2,  // 心蟠
    TIZHI_LINGXI = 1 << 3, // 灵夕
    
    ___COUNT__END____TIZHI____,
    NUM_TIZHI = ___COUNT__END____TIZHI____ >> 1,
};

static const char *GetTizhiName(int ID)
{
    switch (ID)
    {
    case TIZHI_XINSU:   return "心素";
    case TIZHI_XINZHUO: return "心浊";
    case TIZHI_XINPAN:  return "心蟠";
    case TIZHI_LINGXI:  return "灵夕";
    default: return "魄种";
    }
}
#endif