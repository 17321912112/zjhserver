#ifndef ROUND_H
#define ROUND_H

// 游戏开始后的一局游戏
class Round
{
public:
    int getRoundId() const{ return _round_id; }
    int getRoomId() const{ return _room_id; }
    int getPotAmount() const{ return _pot_amount; }
    int getCurrentPlayerId() const{ return _current_playerid; }
    void setCurrentPlayerId(int playerid){ _current_playerid = playerid; }
    void setRoundId(int id){ _round_id = id; }
    void setRoomId(int id){ _room_id = id; }
    void setPotAmount(int amount){ _pot_amount = amount; }
private:
    int _round_id;
    int _room_id;
    int _pot_amount;
    int _current_playerid;
};

#endif