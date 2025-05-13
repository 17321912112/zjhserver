#ifndef ROOMMODEL_H
#define ROOMMODEL_H

#include "room.hpp"
#include "player.hpp"
#include <vector>

class RoomModel
{
public:
    // 新建房间
    int createRoom(int userId, int maxPlayerCount);
    // 删除房间
    bool removeRoom(int roomId);
    // 查询房间
    bool queryRoom(int roomId);
    // 更新房间现有人数
    bool updateCurrentNum(int roomId, int curNum);
    // 获取空闲座位号
    int getFreeSeatPos(int roomId);
    // 解散房间并删除全部玩家
    void destroyRoom(int roomId);
    // 查询房间是否无人
    bool isEmpty(int roomId);
    // 获取房间全部信息
    Room getRoomInfo(int roomId);
    // 获取房间全部玩家id
    std::vector<int> getRoomPlayerIds(int roomId);
    // 获取全部玩家信息
    std::vector<Player> getRoomPlayers(int roomId);
    
};


#endif