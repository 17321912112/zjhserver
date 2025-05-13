#ifndef PUBLIC_H
#define PUBLIC_H

#include <string>
/**
 * server 和 client 公用头文件
 */

enum ActionType
{
    // 看牌、开牌、弃牌、下注
    LOOK_CARD = 1,
    OPEN_CARD,
    FOLD_CARD,
    BET_MONEY,

    LOOK_CARD_ACK,
    OPEN_CARD_ACK,
    FOLD_CARD_ACK,
    BET_MONEY_ACK,

    
};

enum EnMsgType {
    LOGIN_MSG = 1,   // 登录
    REG_MSG,         // 注册
    ONE_CHAT_MSG,        // 聊天消息
    ADD_FRIEND_MSG,  // 添加好友
    CREATE_GROUP_MSG,    // 创建群组
    ADD_GROUP_MSG,  // 加入群组
    GROUP_CHAT_MSG, // 群聊

    REG_MSG_ACK,     // 注册响应
    LOGIN_MSG_ACK,   // 登录响应
    ONE_CHAT_MSG_ACK,    // 聊天消息响应
    ADD_FRIEND_MSG_ACK,   // 添加好友响应
    CREATE_GROUP_MSG_ACK, // 创建群组响应
    ADD_GROUP_MSG_ACK, // 加入群组响应
    GROUP_CHAT_MSG_ACK,  // 群聊消息响应
    LOGINOUT_MSG,   // 退出登录

    // 游戏相关
    CREATE_ROOM_MSG, // 创建房间
    JOIN_ROOM_MSG,   // 加入房间
    LEAVE_ROOM_MSG,  // 离开房间
    CLOSE_ROOM_MSG,  // 解散房间

    PLAYER_JOIN_ROOM_NOTIFY, // 玩家加入房间
    PLAYER_LEAVE_ROOM_NOTIFY, // 玩家离开房间
    ROOM_NOTIFY,

    CREATE_ROOM_MSG_ACK, // 创建房间响应
    JOIN_ROOM_MSG_ACK,   // 加入房间响应
    LEAVE_ROOM_MSG_ACK,  // 离开房间响应
    CLOSE_ROOM_MSG_ACK,  // 解散房间响应

    GAME_NOTIFY, // 游戏通知
    GAME_START_MSG, // 游戏开始
    GAME_START_MSG_ACK, // 游戏开始响应
    PLAYER_ACTION_MSG, // 玩家操作

    PLAYER_ACTION_MSG_ACK, // 玩家操作响应

    GAME_OVER_NOTIFY, 
};

enum RoomState
{
    ROOM_STATE_WAITING = 0, // 等待中
    ROOM_STATE_PLAYING = 1, // 游戏中
};

enum PlayerState
{
    PLAYER_STATE_FOLD = 0, // 弃牌
    PLAYER_STATE_SEEN = 1, // 看牌
    PLAYER_STATE_UNSEEN = 2, // 未看牌
};

enum Card
{
    Card_RedHeart_A = 1,
    Card_RedHeart_2,
    Card_RedHeart_3,
    Card_RedHeart_4,
    Card_RedHeart_5,
    Card_RedHeart_6,
    Card_RedHeart_7,
    Card_RedHeart_8,
    Card_RedHeart_9,
    Card_RedHeart_10,
    Card_RedHeart_J,
    Card_RedHeart_Q,
    Card_RedHeart_K,
    // 黑桃
    Card_BlackHeart_A,
    Card_BlackHeart_2,
    Card_BlackHeart_3,
    Card_BlackHeart_4,
    Card_BlackHeart_5,
    Card_BlackHeart_6,
    Card_BlackHeart_7,
    Card_BlackHeart_8,
    Card_BlackHeart_9,
    Card_BlackHeart_10,
    Card_BlackHeart_J,
    Card_BlackHeart_Q,
    Card_BlackHeart_K,
    // 方块
    Card_Square_A,
    Card_Square_2,
    Card_Square_3,
    Card_Square_4,
    Card_Square_5,
    Card_Square_6,
    Card_Square_7,
    Card_Square_8,
    Card_Square_9,
    Card_Square_10,
    Card_Square_J,
    Card_Square_Q,
    Card_Square_K,
    // 梅花
    Card_Plum_A,
    Card_Plum_2,
    Card_Plum_3,
    Card_Plum_4,
    Card_Plum_5,
    Card_Plum_6,
    Card_Plum_7,
    Card_Plum_8,
    Card_Plum_9,
    Card_Plum_10,
    Card_Plum_J,
    Card_Plum_Q,
    Card_Plum_K,
    Card_End,
};

#endif