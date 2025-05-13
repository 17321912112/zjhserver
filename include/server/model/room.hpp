#ifndef ROOM_H
#define ROOM_H

#include <string>
#include "public.hpp"

class Room
{
public:


    Room()
        :_maxPlayer(6),
        _curPlayer(1)
    {};    
    ~Room(){};

    // 房间id
    int getRoomId() const { return _roomId; }
    // 房间所有者id
    int getOwner() const { return _roomOwner; }
    // 当前人数
    int getCurrentPlayerCount() const { return _curPlayer; }
    // 最大人数
    int getMaxPlayerCount() const { return _maxPlayer; }
    // 房间状态
    RoomState getRoomState() const { return _roomState; }

    // 设置房间id
    void setRoomId(int roomId) { _roomId = roomId; }
    // 设置房间所有者id
    void setOwner(int owner) { _roomOwner = owner; }
    // 设置最大人数
    void setMaxPlayerCount(int maxPlayer) { _maxPlayer = maxPlayer; }
    // 设置当前人数
    void setCurrentPlayerCount(int curPlayer) { _curPlayer = curPlayer; }
    // 设置房间状态
    void setRoomState(RoomState state) { _roomState = state; }
private:
    int _roomId; // 房间id
    int _roomOwner; // 房间所有者id
    int _maxPlayer; // 最大人数
    int _curPlayer; // 当前人数
    RoomState _roomState; // 房间状态
};

#endif