#ifndef PLAYER_H
#define PLAYER_H
#include <string>
#include "public.hpp"
class Player
{
public:
    Player(int userid, int roomId)
        : _userid(userid), _roomId(roomId),_state("UNSEEN") {}

    Player():_state("UNSEEN"){}
    ~Player() {}

    int getUserId() const { return _userid; }
    int getRoomId() const { return _roomId; }
    int getSeatPos() const { return _seatPos; }
    std::string getCards() const { return _cards; }
    std::string getStatus() const { return _state; }
    int getScore() const { return _score; }

    void setUserId(int userid) { _userid = userid; }
    void setRoomId(int roomId) { _roomId = roomId; }
    void setSeatPos(int seatPos) { _seatPos = seatPos; }
    void setCards(const std::string& cards) { _cards = cards; }
    void setState(std::string state) { _state = state; }

    void setScore(int score) { _score = score; }

    std::string getName() const { return _name; }
    void setName(const std::string& name) { _name = name; }
private:
    int _roomId;     // 房间id
    int _seatPos;   // 座位号
    int _userid;     // 玩家id
    std::string _cards; // 手牌
    std::string _state; // 玩家状态
    int _score;     // 分数
    std::string _name; // 玩家名字
};


#endif