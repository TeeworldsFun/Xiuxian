#ifndef GAME_SERVER_TIZHIDATA_H
#define GAME_SERVER_TIZHIDATA_H

enum
{
    TIZHI_NONE = 0,         // 没有特殊体质
    TIZHI_XINSU = 1 << 0,   // 心素
    TIZHI_XINZHUO = 1 << 1, // 心浊
    TIZHI_XINPAN = 1 << 2,  // 心蟠
    TIZHI_LINGXI = 1 << 3, // 灵夕
};

#endif