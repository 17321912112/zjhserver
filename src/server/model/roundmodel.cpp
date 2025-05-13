#include "roundmodel.hpp"
#include "db.h"

int RoundModel::addRound(const Round& round)
{
    char sql[1024];
    int inserted_id = -1;
    sprintf(sql, "INSERT INTO game_rounds (room_id, pot_amount, current_playerid) VALUES (%d, %d, %d)",
            round.getRoomId(), round.getPotAmount(), round.getCurrentPlayerId());
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            sprintf(sql, "SELECT LAST_INSERT_ID() AS id");
            MYSQL_RES* res = mysql.query(sql);
            if (res != nullptr)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                inserted_id = atoi(row[0]);
                mysql_free_result(res); // 释放结果集
            }
        }
    }
    return inserted_id;
}
// 更新对局信息 需要 roundid, pot_amount, current_player
bool RoundModel::updateRound(const Round& round)
{
    char sql[1024];
    sprintf(sql, "UPDATE game_rounds SET pot_amount = %d, current_playerid = %d WHERE round_id = %d",
            round.getPotAmount(), round.getCurrentPlayerId(), round.getRoundId());
    MySQL mysql;
    if (mysql.connect())
    {
        return mysql.update(sql);
    }
    return false;
}

Round RoundModel::getRound(int roundid)
{
    char sql[1024];
    Round round;
    sprintf(sql, "SELECT room_id,pot_amount,current_playerid FROM game_rounds WHERE round_id = %d", roundid);
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            
            round.setRoomId(atoi(row[0]));
            round.setPotAmount(atoi(row[1]));
            round.setCurrentPlayerId(atoi(row[2]));
            round.setRoundId(roundid);
            mysql_free_result(res); // 释放结果集
            return round;
        }
    }
    return round;
}