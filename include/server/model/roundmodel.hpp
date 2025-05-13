#ifndef ROUNDMODEL_H
#define ROUNDMODEL_H

#include "round.hpp"

class RoundModel
{
public:
    // 新增对局 需要 roomid, pot_amount, current_player
    int addRound(const Round& round);
    // 更新对局信息 需要 roundid, pot_amount, current_player
    bool updateRound(const Round& round);
    // 获取当前对局信息 需要 roundid
    Round getRound(int roundid);
};



#endif