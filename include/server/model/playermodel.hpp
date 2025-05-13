#ifndef PLAYERMODEL_H
#define PLAYERMODEL_H

#include "player.hpp"
#include <vector>

class PlayerModel
{
public:
    // 玩家加入房间
    bool userJoinRoom(int roomId, int userId, int seatPos);
    // 玩家离开房间
    bool userLeaveRoom(int roomId, int userId);
    // 获取房间内所有玩家信息
    std::vector<Player> getPlayersInRoom(int roomId);
    // 更新玩家信息
    bool updatePlayerInfo(const Player& player);
    // 获取玩家信息
    Player getPlayerInfo(int roomId, int userId);
    // 获取所有未弃牌玩家
    std::vector<Player> getNotFoldPlayers(int roomId);
    
};

#endif