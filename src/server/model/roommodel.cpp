#include "roommodel.hpp"
#include "db.h"
#include "player.hpp"
#include <unordered_set>
#include <algorithm>
#include <vector>

// 新建房间
int RoomModel::createRoom(int userId, int maxPlayerCount = 6)
{
    char sql[1024]{0};
    sprintf(sql, "insert into rooms(owner_id, max_players, current_players) values (%d, 6, 1)", userId);
    int roomId = -1;
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            // 获取房间id
            sprintf(sql, "select last_insert_id()");
            MYSQL_RES *res = mysql.query(sql);
            if (res != nullptr)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row != nullptr)
                {
                    roomId = atoi(row[0]);
                    mysql_free_result(res); // 释放结果集
                }
            }
        }
    }
    return roomId;
}
// 删除房间
bool RoomModel::removeRoom(int roomId)
{
    char sql[1024]{0};
    sprintf(sql, "delete from rooms where room_id = %d", roomId);

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

bool RoomModel::queryRoom(int roomId)
{
    char sql[1024]{0};
    sprintf(sql, "select owner_id from rooms where room_id = %d", roomId);

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                mysql_free_result(res); // 释放结果集
                return true;
            }
        }
    }
    return false;
}

int RoomModel::getFreeSeatPos(int roomId)
{
    char sql[1024]{0};
    // 查出房间座位信息
    sprintf(sql, "select a.seat_pos, b.max_players from room_players a inner join rooms b on a.room_id = b.room_id where a.room_id = %d", roomId);
    MySQL mysql;
    int maxPlayer = 0;
    int pos = -1;
    std::unordered_set<int> occupiedSeats;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                occupiedSeats.insert(atoi(row[0]));
                maxPlayer = atoi(row[1]);
            }
            mysql_free_result(res); // 释放结果集
        }
    }

    if (occupiedSeats.size() < maxPlayer)
    {
        for (int i = 1; i <= maxPlayer; ++i)
        {
            if (occupiedSeats.find(i) == occupiedSeats.end())
            {
                pos = i;
                break;
            }
        }
    }
    return pos;
}

bool RoomModel::updateCurrentNum(int roomId, int curNum)
{
    char sql[1024]{0};
    sprintf(sql, "update rooms set current_players = %d where room_id = %d", curNum, roomId);

    MySQL mysql;
    if (mysql.connect())
    {
        return mysql.update(sql);
    }
    return false;
}

void RoomModel::destroyRoom(int roomId)
{
    char sql[1024]{0};
    sprintf(sql, "delete from room_players where room_id = %d", roomId);
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            sprintf(sql, "delete from rooms where room_id = %d", roomId);
            mysql.update(sql);
        }
    }
}

bool RoomModel::isEmpty(int roomId)
{
    char sql[1024]{0};
    sprintf(sql, "select current_players from rooms where room_id = %d", roomId);
    MySQL mysql;
    int curNum = 0;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                curNum = atoi(row[0]);
                mysql_free_result(res); // 释放结果集
            }
        }
    }
    return curNum == 0;
}

Room RoomModel::getRoomInfo(int roomId)
{
    char sql[1024]{0};
    sprintf(sql, "select room_id, owner_id, max_players, current_players, status from rooms where room_id = %d", roomId);
    MySQL mysql;
    Room room{};
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                room.setRoomId(atoi(row[0]));
                room.setOwner(atoi(row[1]));
                room.setMaxPlayerCount(atoi(row[2]));
                room.setCurrentPlayerCount(atoi(row[3]));
                room.setRoomState(static_cast<RoomState>(atoi(row[4])));
                mysql_free_result(res); // 释放结果集
            }
        }
    }
    return room;
}

std::vector<int> RoomModel::getRoomPlayerIds(int roomId)
{
    char sql[1024]{0};
    sprintf(sql, "select user_id from room_players where room_id = %d", roomId);
    MySQL mysql;
    std::vector<int> ids{};
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                ids.push_back(atoi(row[0]));
            }
            mysql_free_result(res); // 释放结果集
        }
    }
    return ids;
}

std::vector<Player> RoomModel::getRoomPlayers(int roomId)
{
    char sql[1024]{0};
    sprintf(sql, "select a.user_id, b.name, a.seat_pos, a.score from room_players a inner join \
                    User b on a.user_id = b.id where a.room_id = %d", roomId);
    MySQL mysql;
    std::vector<Player> players{};
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                Player player{};
                player.setUserId(atoi(row[0]));
                player.setName(std::string(row[1]));
                player.setSeatPos(atoi(row[2]));
                player.setScore(atoi(row[3]));
                players.push_back(player);
            }
        }
    }
    return players;
}