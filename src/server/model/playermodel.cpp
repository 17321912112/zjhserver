#include "playermodel.hpp"
#include "db.h"

// 玩家加入房间
bool PlayerModel::userJoinRoom(int roomId, int userId, int seatPos)
{
    char sql[1024]{0};
    sprintf(sql, "insert into room_players(room_id,seat_pos, user_id) values (%d,%d, %d)", roomId, seatPos, userId);
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}
// 玩家离开房间
bool PlayerModel::userLeaveRoom(int roomId, int userId)
{
    char sql[1024]{0};
    sprintf(sql, "delete from room_players where room_id = %d and user_id = %d", roomId, userId);
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}
// 获取房间内所有玩家信息
std::vector<Player> PlayerModel::getPlayersInRoom(int roomId)
{   
    std::vector<Player> players;
    char sql[1024]{0};
    sprintf(sql, "select a.user_id, a.seat_pos, a.cards, a.status, a.score, b.name from room_players a inner join\
            User b on a.user_id=b.id where a.room_id = %d", roomId);
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                Player player;
                player.setUserId(atoi(row[0]));
                player.setSeatPos(atoi(row[1]));
                if (row[2]){
                    player.setCards(row[2]);
                }
                player.setState(row[3]);
                player.setScore(atoi(row[4]));
                player.setName(row[5]);
                player.setRoomId(roomId);
                players.push_back(player);
            }
            mysql_free_result(res); // 释放结果集
        }
    }
    return players;

}
// 更新玩家信息
bool PlayerModel::updatePlayerInfo(const Player& player)
{
    char sql[1024]{0};
    sprintf(sql, "update room_players set seat_pos = %d, cards = '%s', status = '%s', score = %d where user_id = %d",
        player.getSeatPos(), player.getCards().c_str(), player.getStatus().c_str(), player.getScore(), player.getUserId());
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}
// 获取玩家信息
Player PlayerModel::getPlayerInfo(int roomId, int userId)
{
    Player player;
    char sql[1024]{0};
    sprintf(sql, "select user_id, seat_pos, cards, status, score from room_players where room_id = %d and user_id = %d", roomId, userId);
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            player.setUserId(atoi(row[0]));
            player.setSeatPos(atoi(row[1]));
            player.setCards(row[2]);    
            player.setState(row[3]);
            player.setScore(atoi(row[4]));
            mysql_free_result(res); // 释放结果集
        }
    }
    return player;
}


std::vector<Player> PlayerModel::getNotFoldPlayers(int roomId)
{
    std::vector<Player> players;
    char sql[1024]{0};
    sprintf(sql, "select user_id, seat_pos, cards, status, score from room_players where room_id = %d and status!='FOLD'", roomId);
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                Player player;
                player.setUserId(atoi(row[0]));
                player.setSeatPos(atoi(row[1]));
                if (row[2]){
                    player.setCards(row[2]);
                }
                player.setState(row[3]);
                player.setScore(atoi(row[4]));
                player.setRoomId(roomId);
                players.push_back(player);
            }
            mysql_free_result(res); // 释放结果集
        }
    }
    return players;
}