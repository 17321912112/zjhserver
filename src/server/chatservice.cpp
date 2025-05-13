#include "chatservice.hpp"
#include "public.hpp"
#include "redis.hpp"
#include "gameservice.hpp"
#include <muduo/base/Logging.h>
#include <mutex>

ChatService* ChatService::getInstance()
{
    static ChatService instance;
    return &instance;
}
// 获取消息对应的处理器
MsgHandler ChatService::getMsgHandler(int msgid)
{
    if (_msgHandlerMap.find(msgid) != _msgHandlerMap.end())
    {
        return _msgHandlerMap[msgid];
    }
    else
    {
        return [msgid](const TcpConnectionPtr& , json& , Timestamp){
            LOG_ERROR << "msgid:" << msgid << "handler is not found!";
        };
    }
}
ChatService::ChatService() 
    :_msgHandlerMap()
    ,_userModel()
    ,_redis()
{
    // 注册消息处理回调
    _msgHandlerMap.insert({ LOGIN_MSG, std::bind(&ChatService::login, this, ::_1, ::_2, ::_3) });
    _msgHandlerMap.insert({ REG_MSG, std::bind(&ChatService::reg, this, ::_1, ::_2, ::_3) });
    _msgHandlerMap.insert({ ONE_CHAT_MSG, std::bind(&ChatService::oneToOneChat, this, ::_1, ::_2, ::_3) });
    _msgHandlerMap.insert({ ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, ::_1, ::_2, ::_3) });
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, ::_1, ::_2, ::_3) });
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::joinGroup, this, ::_1, ::_2, ::_3) });
    _msgHandlerMap.insert({ GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, ::_1, ::_2, ::_3) });
    _msgHandlerMap.insert({ CREATE_ROOM_MSG, std::bind(&ChatService::createRoom, this, ::_1, ::_2, ::_3) });
    _msgHandlerMap.insert({ JOIN_ROOM_MSG, std::bind(&ChatService::joinRoom, this, ::_1, ::_2, ::_3) });
    _msgHandlerMap.insert({ LEAVE_ROOM_MSG, std::bind(&ChatService::leaveRoom, this, ::_1, ::_2, ::_3) });
    _msgHandlerMap.insert({ CLOSE_ROOM_MSG, std::bind(&ChatService::destroyRoom, this, ::_1, ::_2, ::_3) });
    _msgHandlerMap.insert({ GAME_START_MSG, std::bind(&ChatService::startGame, this, ::_1, ::_2, ::_3) });
    _msgHandlerMap.insert({ PLAYER_ACTION_MSG, std::bind(&ChatService::playerAction, this, ::_1, ::_2, ::_3) });

    if (_redis.connect())
    {
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, ::_1, ::_2));
    }
}
// 处理登录业务
void ChatService::login(const TcpConnectionPtr& conn, json& js, Timestamp time) 
{
    LOG_INFO << "do login...";
    int id = js["id"];
    string password = js["password"];
    User user = _userModel.query(id);
    json response;
    response["msgid"] = LOGIN_MSG_ACK;
    if (user.online())
    {
        response["errno"] = 1;
        response["errmsg"] = "用户已登录,不允许重复登陆!";
        conn->send(response.dump());
    }
    else if(user.getId() == id && user.getPassword() == password) 
    {   
        response["errno"] = 0;
        response["id"] = id;
        response["name"] = user.getName();
        // 登录成功,修改用户状态
        user.setState("online");
        _userModel.updateState(user);
        // 用户登陆后,订阅该用户的消息
        _redis.subscribe(id);
        // 读离线消息
        vector<string> offlineMsgs = _offlineMsgModel.query(id);
        if (!offlineMsgs.empty())
        {
            response["offlinemsg"] = offlineMsgs;
            // 读取该用户的离线消息后，把该用户的所有离线消息删除掉
            _offlineMsgModel.remove(id);
        }

        // 显示好友列表
        vector<User> friends = _friendModel.getFriends(id);
        if (!friends.empty())
        {
            vector<string> friendList;
            for (auto& user : friends)
            {
                json js;
                js["id"] = user.getId();
                js["name"] = user.getName();
                js["state"] = user.getState();
                friendList.push_back(js.dump());
            }
            response["friends"] = friendList;
        }
        // 显示群组列表
        vector<Group> groups = _groupModel.queryGroups(id);
        vector<string> groupList;
        if (!groups.empty())
        {
            for (auto& group : groups)
            {
                json js;
                js["id"] = group.getId();
                js["groupname"] = group.getName();
                js["groupdesc"] = group.getDesc();
                vector<string> users;
                vector<GroupUser> usersInGroup = group.getUsers();
                for (auto& u : usersInGroup)
                {
                    json j;
                    j["id"] = u.getId();
                    j["name"] = u.getName();
                    j["state"] = u.getState();
                    j["role"] = u.getRole();
                    users.emplace_back(j.dump());
                }
                js["users"] = users;
                groupList.push_back(js.dump());
            }
            response["groups"] = groupList;
        }
        // 发送登录成功消息到客户端
        LOG_INFO << "send:" << response.dump();
        conn->send(response.dump());
        // 记录用户的连接
        {
            std::lock_guard<std::mutex> lock(_connMutex);
            _userConnMap.insert({ id, conn });
        }
    }
    else
    {
        response["errno"] = 1;
        response["errmsg"] = "用户名或密码错误!";
        conn->send(response.dump());
    }
}
// 处理注册业务
void ChatService::reg(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    LOG_INFO << "do reg...";
    User user;
    user.setName(js["name"]);
    user.setPassword(js["password"]);
    json response;
    if(_userModel.insert(user))
    {
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
    }
    else 
    {
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "注册失败!";
    }
    conn->send(response.dump());
}

// 客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr& conn)
{
    User user;
    {
        std::lock_guard<std::mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
        _redis.unsubscribe(user.getId());
    }
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
        LOG_INFO << "Client: " << conn->peerAddress().toIpPort() << " offline!";
    }
    conn->shutdown();
}

void ChatService::oneToOneChat(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    string msg = js["msg"];
    int id = js["toid"].get<int>(); 
    {
        // 找目标是否在线
        json response;
        response["msgid"] = ONE_CHAT_MSG;
        std::lock_guard<mutex> lock(_connMutex);
        response["id"] = js["id"];
        response["msg"] = js["msg"];
        response["time"] = js["time"];
        response["name"] = js["name"];
        if (_userConnMap.find(id) != _userConnMap.end())
        {
            _userConnMap[id]->send(js.dump());
        }
        else 
        {
            // 查是否在线
            User user = _userModel.query(id);
            if (user.getState() == "offline")
            {
                _offlineMsgModel.insert(id, js.dump());
            }
            else
            {
                _redis.publish(id, js.dump());
            }
        }
    }
}

void ChatService::reset()
{
    _userModel.resetState();
}

void ChatService::addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();
    json response;
    response["msgid"] = ADD_FRIEND_MSG_ACK;
    if (_friendModel.insert(userid, friendid))
    {
        response["errno"] = 0;
        response["msg"] = "添加好友成功!";
    }
    else
    {
        response["errno"] = 1;
        response["msg"] = "添加好友失败!";
    }
    conn->send(response.dump());
}
// 创建群组
void ChatService::createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    Group group;
    string groupname = js["groupname"];
    string groupdesc = js["groupdesc"];
    json response;
    response["msgid"] = CREATE_GROUP_MSG_ACK;
    group.setName(groupname);
    group.setDesc(groupdesc);
    if (_groupModel.insert(group))
    {
        // 群组加入自己
        _groupModel.addToGroup(group.getId(), js["id"].get<int>(), "creator");
        response["errno"] = 0;
        response["msg"] = "创建群组成功!";
        response["groupid"] = group.getId();
    }
    else
    {
        response["errno"] = 1;
        response["msg"] = "创建群组失败!";
    }
    conn->send(response.dump());
}
// 群聊
void ChatService::groupChat(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int groupid = js["groupid"].get<int>();
    int senderid = js["id"].get<int>();
    string msg = js["msg"];
    json response;
    response["msgid"] = GROUP_CHAT_MSG;
    vector<int> memberids = _groupModel.queryGroupUsers(senderid, groupid);
    {
        std::lock_guard<std::mutex> lock(_connMutex);
        response["id"] = senderid;
        response["msg"] = msg;
        response["groupid"] = groupid;
        response["time"] = js["time"];
        response["name"] = js["name"];
        for (auto id : memberids)
        {
            if (_userConnMap.find(id) != _userConnMap.end())
            {
                _userConnMap[id]->send(response.dump());
            }
            else
            {
                // 不在线放入离线消息中
                if (_userModel.query(id).getState() == "offline")
                {
                    _offlineMsgModel.insert(id, response.dump());
                }
                else
                {
                    _redis.publish(id, response.dump());
                }
            }
        }
    }
}
// 加入群组
void ChatService::joinGroup(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userid = js["userid"].get<int>();
    int groupid = js["groupid"].get<int>();
    json response;
    if (_groupModel.addToGroup(groupid, userid, "normal"))
    {
        response["msgid"] = ADD_GROUP_MSG_ACK;
        response["errno"] = 0;
        response["msg"] = "加入群组成功!";
    }
    else
    {
        response["msgid"] = ADD_GROUP_MSG_ACK;
        response["errno"] = 1;
        response["msg"] = "加入群组失败!";
    }
    conn->send(response.dump());
}

void ChatService::handleRedisSubscribeMessage(int channel, string message)
{
    // 处理redis订阅的消息
    json js = json::parse(message);
    std::lock_guard<std::mutex> lock(_connMutex);
    if (_userConnMap.find(channel) != _userConnMap.end())
    {
        _userConnMap[channel]->send(message);
    }
    else
    {
        // 不在线放入离线消息中
        _offlineMsgModel.insert(channel, message);
    }
}

void ChatService::createRoom(const TcpConnectionPtr& conn, json& js, Timestamp time)
{

    /*
        统一返回 json : {"msgid": ?, "errno": ?, "errmsg": ?, "roomid":?, "currentplayers":? , "maxplayers": ? ,
             "players":{{"userid":?, "username:"?, "seatpos":?, "score":?}, {...} }   }
    */
    int userid, maxplayers;
    parse(js, "userid", userid);
    parse(js, "maxplayers", maxplayers);
    json response;
    response["msgid"] = CREATE_ROOM_MSG_ACK;
    int roomId = _roomModel.createRoom(userid, maxplayers);
    if (-1 != roomId)
    {
        // 加入房间
        _playerModel.userJoinRoom(roomId, userid, 1);
        response["errno"] = 0;
        response["roomid"] = roomId;
        response["maxplayers"] = maxplayers;
        response["currentplayers"] = 1;
        User user = _userModel.query(userid);
        json playersJson = response;
        playersJson["userid"] = userid;
        playersJson["username"] = user.getName();
        playersJson["seatpos"] = 1;
        playersJson["score"] = 0;
        response["players"].push_back(std::move(playersJson));
        json notifyJson;
        notifyJson = response;
        notifyJson["msgid"] = ROOM_NOTIFY;
        notify(notifyJson, {userid});
    }
    else
    {
        response["errno"] = 1;
        response["errmsg"] = "创建房间失败!";
        conn->send(response.dump());
    }
}
// 加入房间
void ChatService::joinRoom(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    /*
        统一返回 json : {"msgid": ?, "errno": ?, "errmsg": ?, "roomid":?, "currentplayers":? , "maxplayers": ? ,
             "players":{{"userid":?, "username:"?, "seatpos":?, "score":?}, {...} }   }
    */
    int userid = js["userid"].get<int>();
    int roomid = js["roomid"].get<int>();
    json response;
    response["msgid"] = JOIN_ROOM_MSG_ACK;
    Room room;
    if (!_roomModel.queryRoom(roomid))
    {
        response["errno"] = 1;
        response["errmsg"] = "房间不存在!"; //加入房间后分配一个座位
        conn->send(response.dump());
        return ;
    }
    else
    {
        // 1.更新房间信息和玩家信息
        // 2.加入到房间的玩家列表中
        // 3.通知房间内其他玩家有加入房间信息
        room = _roomModel.getRoomInfo(roomid);
        if(_roomModel.updateCurrentNum(roomid, room.getCurrentPlayerCount() + 1))
        {
            // 分配座位
            int seatpos = _roomModel.getFreeSeatPos(roomid);
            if (seatpos != -1)
            {
                if (_playerModel.userJoinRoom(roomid, userid, seatpos))
                {
                    response["errno"] = 0;
                    // 通知房间内其他玩家有加入房间信息 {"msgid":, "players":{{"userid", "seatpos", "name", "score"}, {"userid", "seatpos"}}
                    
                    response["roomid"] = roomid;
                    response["maxplayers"] = room.getMaxPlayerCount() + 1;
                    response["currentplayers"] = room.getCurrentPlayerCount() + 1;
                    json notifyJson = response;
                    notifyJson["errno"] = 0;
                    notifyJson["msgid"] = ROOM_NOTIFY;
                    
                    std::vector<int> playerids = _roomModel.getRoomPlayerIds(roomid);
                    std::vector<Player> players = _roomModel.getRoomPlayers(roomid);
                    for (auto& player : players)
                    {
                        json playerJson;
                        playerJson["userid"] = player.getUserId();
                        playerJson["seatpos"] = player.getSeatPos();
                        playerJson["name"] = player.getName();
                        playerJson["score"] = player.getScore();
                        notifyJson["players"].push_back(std::move(playerJson));
                    }
                    notify(notifyJson, playerids);
                    return ;
                }
                else
                {
                    _roomModel.updateCurrentNum(roomid, room.getCurrentPlayerCount());
                }
            }
        }
    }
    response["errno"] = 1;
    response["errmsg"] = "加入房间失败!";
    conn->send(response.dump());
}
// 退出房间
void ChatService::leaveRoom(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    /*
        统一返回 json : {"msgid": ?, "errno": ?, "errmsg": ?, "roomid":?, "currentplayers":? , "maxplayers": ? ,
             "players":{{"userid":?, "username:"?, "seatpos":?, "score":?}, {...} }   }
    */
    int userid = js["userid"].get<int>();
    int roomid = js["roomid"].get<int>();
    json response;
    response["msgid"] = LEAVE_ROOM_MSG_ACK;
    if (_playerModel.userLeaveRoom(roomid, userid))
    {
        Room room = _roomModel.getRoomInfo(roomid);
        _roomModel.updateCurrentNum(roomid, room.getCurrentPlayerCount() - 1);
        if (room.getCurrentPlayerCount() - 1 != 0)
        {
            response["errno"] = 0;
            // 通知房间内其他玩家有加入房间信息 {"msgid":, "players":{{"userid", "seatpos", "name", "score"}, {"userid", "seatpos"}}
            response["roomid"] = roomid;
            response["maxplayers"] = room.getMaxPlayerCount();
            response["currentplayers"] = room.getCurrentPlayerCount() - 1;
            json notifyJson = response;
            json playersJson;
            notifyJson["msgid"] = ROOM_NOTIFY;
            
            std::vector<int> playerids = _roomModel.getRoomPlayerIds(roomid);
            std::vector<Player> players = _roomModel.getRoomPlayers(roomid);
            for (auto& player : players)
            {
                json playerJson;
                playerJson["userid"] = player.getUserId();
                playerJson["seatpos"] = player.getSeatPos();
                playerJson["name"] = player.getName();
                playerJson["score"] = player.getScore();
                playersJson.push_back(std::move(playerJson));
            }
            notifyJson["players"] = playersJson;
            notify(notifyJson, playerids);
        }
        if (_roomModel.isEmpty(roomid))
        {
            _roomModel.destroyRoom(roomid);
        }
        response["errno"] = 0;

    }
    else
    {
        response["errno"] = 1;
    }
    conn->send(response.dump());
}

void ChatService::destroyRoom(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    // req :{"msgid":19,"userid":1, "roomid":2}
    // response :{"msgid":14,"errno":0}
    int userid = js["userid"].get<int>();
    int roomid = js["roomid"].get<int>();
    json response;
    response["msgid"] = CLOSE_ROOM_MSG_ACK;
    _roomModel.destroyRoom(roomid);
    response["errno"] = 0;
    conn->send(response.dump());
}

void ChatService::notify(const json& js, const vector<int>& userIds, int exceptUserId)
{
    LOG_DEBUG << "notify:" << js.dump();
    std::lock_guard<std::mutex> lock(_connMutex);
    for (auto id : userIds)
    {
        if (id != exceptUserId && _userConnMap.find(id) != _userConnMap.end())
        {
            LOG_INFO << "notify userid:" << id;
            _userConnMap[id]->send(js.dump());
        }
    }
}

template<typename T>
bool ChatService::parse(json& js, const string& key, T& val)
{
    if (js.contains(key))
    {
        val = js[key].get<T>();
        return true;
    }
    return false;
}

void ChatService::startGame(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    /*
        接收： {"msgid": 20, "userid":1,"roomid":1}
        返回:  {"msgid": 21, "errno":0}
        通知:  {"msgid": 22, "players":[{userid:, seatpos:, name:, score:}, {userid:, seatpos:, name:, score:}], "msg":"", 
        "currentplayerid":, "total":, "roundid":}
    */
    int userId, roomId;
    parse(js, "userid", userId);
    parse(js, "roomid", roomId);
    json response;
    response["msgid"] = GAME_START_MSG_ACK;

    json notifyJson;
    notifyJson["msgid"] = GAME_NOTIFY;

    std::vector<Player> players = _roomModel.getRoomPlayers(roomId);
    if (players.size() == 0)
    {
        response["errno"] = 1;
        response["errmsg"] = "房间中没有玩家!";
        conn->send(response.dump());
        return ;
    }
    GameService::getInstance().initCards();
    int total = 0;
    int currentPlayerId = players[rand() % players.size()].getUserId();
    for (Player &player : players)
    {
        // 发牌->存入数据库, 积分-1
        std::string cardStr;
        std::vector<Card> cards = GameService::getInstance().getRandomCards();
        cardStr = std::to_string(cards[0]) + ',' + std::to_string(cards[1]) + ',' + std::to_string(cards[2]);
        player.setCards(cardStr);
        player.setScore(player.getScore() - 1);
        if (!_playerModel.updatePlayerInfo(player))
        {
            LOG_ERROR  << "更新玩家信息失败!";
            response["errno"] = 1;
            conn->send(response.dump());
            return ;
        }

        total++;

        // 组装notifyer
        json playerJson;
        playerJson["userid"] = player.getUserId();
        playerJson["seatpos"] = player.getSeatPos();
        playerJson["name"] = player.getName();
        playerJson["score"] = player.getScore();
        notifyJson["players"].push_back(playerJson);
    }
    notifyJson["currentplayerid"] = currentPlayerId;
    notifyJson["msg"] = "玩家id: " + std::to_string(userId) + "，开始了游戏! 第一个行动的玩家id:" + to_string(currentPlayerId);
    
    // 操作游戏轮次
    Round round;
    round.setRoomId(roomId);
    round.setCurrentPlayerId(currentPlayerId);
    round.setPotAmount(total);
    notifyJson["pot"] = total;
    int roundId = _roundModel.addRound(round);
    if (roundId == -1)
    {
        LOG_ERROR << "添加游戏轮次失败!";
        response["errno"] = 1;
        conn->send(response.dump());
        return ;
    }
    notifyJson["roundid"] = roundId;
    notify(notifyJson, _roomModel.getRoomPlayerIds(roomId));
    response["errno"] = 0;
    conn->send(response.dump());
}

void ChatService::playerAction(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    /*
        接收： {"msgid": 23, "userid":1,"roomid":1}
        返回:  {"msgid": 24, "errno":0}
        通知:  {"msgid": 25, "players":[{userid:, seatpos:, name:, score:}, {userid:, seatpos:, name:, score:}], "msg":"", 
        "nextid":, "total":, "roundid":}
    */
    if (!js.contains("actionid"))
    {
        LOG_INFO << "缺少actionid!";
        return ;
    }

    switch (js["actionid"].get<int>())
    {
        case LOOK_CARD:
            peek(conn, js);
            break;
        case OPEN_CARD:
            openCard(conn, js);
            break;
        case FOLD_CARD:
            foldCard(conn, js);
            break;
        case BET_MONEY:
            bet(conn, js);
            break;
        default:
            break;
    }
    
}

// 看牌
void ChatService::peek(const TcpConnectionPtr& conn, json& js)
{
    /*
        接收： {"msgid","actionid","userid","roomid","roundid"}
        返回:  {"msgid": 24,"actionid", "errno":0, "cards":"1, 2, 3"}
        通知:  {"msgid","msg"}
    */
    int userId, roomId, roundId;
    json response;
    response["msgid"] = PLAYER_ACTION_MSG_ACK;
    if (!parse(js, "userid", userId) || !parse(js, "roomid", roomId) || !parse(js, "roundid", roundId))
    {
        response["errno"] = 1;
        response["errmsg"] = "缺少参数!";
        conn->send(response.dump());
        LOG_ERROR << "缺少参数!";
        return ;
    }
    // 看牌后仅变更玩家状态
    Player player = _playerModel.getPlayerInfo(roomId, userId);
    player.setState("SEEN");
    if (!_playerModel.updatePlayerInfo(player))
    {
        response["errno"] = 1;
        response["errmsg"] = "更新玩家信息失败!";
        conn->send(response.dump());
        LOG_ERROR << "更新玩家信息失败!";
        return ;
    }
    json notifyJson;
    notifyJson["msgid"] = GAME_NOTIFY;
    notifyJson["msg"] = "玩家id: " + std::to_string(userId) + ", 看牌了!";

    notify(notifyJson, _roomModel.getRoomPlayerIds(roomId));

    response["actionid"] = LOOK_CARD_ACK;
    response["cards"] = player.getCards();
    response["errno"] = 0;
    conn->send(response.dump());
}
// 弃牌
void ChatService::foldCard(const TcpConnectionPtr& conn, json& js)
{
    /*
        接收： {"msgid","actionid","userid","roomid","roundid"}
        返回:  {"msgid": 24,"actionid", "errno":0}
        通知:  {"msgid","msg"}
    */
    int userId, roomId, roundId;
    json response;
    response["msgid"] = PLAYER_ACTION_MSG_ACK;
    if (!parse(js, "userid", userId) || !parse(js, "roomid", roomId) || !parse(js, "roundid", roundId))
    {
        response["errno"] = 1;
        response["errmsg"] = "缺少参数!";
        conn->send(response.dump());
        LOG_ERROR << "缺少参数!";
        return ;
    }
    // FOLD牌
    Player curPlayer = _playerModel.getPlayerInfo(roomId, userId);
    curPlayer.setState("FOLD");
    if (!_playerModel.updatePlayerInfo(curPlayer))
    {
        response["errno"] = 1;
        response["errmsg"] = "FOLD:更新玩家信息失败!";
        conn->send(response.dump());
        LOG_ERROR << "FOLD:更新玩家信息失败!";
        return ;
    }

    json notifyJson;
    notifyJson["msgid"] = GAME_NOTIFY;
    
    response["actionid"] = FOLD_CARD_ACK;
    response["errno"] = 0;

    if (isGameOver(roomId))
    {
        gameOver(conn, roomId, roundId);
        return ;
    }
    // 获取下一个行动的玩家id
    int nextPlayerId = getNextActionPlayerId(roomId, roundId);
    if (!updateRound(roundId, nextPlayerId))
    {
        LOG_ERROR << "更新游戏轮次失败!";
        response["errno"] = 1;
        conn->send(response.dump());
        return ;
    }
    notifyJson["currentplayerid"] = nextPlayerId;
    notifyJson["msg"] = "玩家id: " + std::to_string(userId) + ", 弃牌了!\n 下一个行动的玩家id: " + to_string(nextPlayerId);
    
    notify(notifyJson, _roomModel.getRoomPlayerIds(roomId));
    conn->send(response.dump());
}
// 开牌
void ChatService::openCard(const TcpConnectionPtr& conn, json& js)
{

}
// 下注
void ChatService::bet(const TcpConnectionPtr& conn, json& js)
{
    /*
        接收： {"msgid","actionid","userid","roomid","roundid", "bet"}
    */
    int userId, roomId, roundId, bet;
    json response;
    response["msgid"] = PLAYER_ACTION_MSG_ACK;
    if (!parse(js, "userid", userId) || !parse(js, "roomid", roomId) || !parse(js, "roundid", roundId) || !parse(js, "bet", bet))
    {
        response["errno"] = 1;
        response["errmsg"] = "缺少参数!";
        conn->send(response.dump());
        LOG_ERROR << "缺少参数!";
        return ;
    }
    // 计算积分
    Player curPlayer = _playerModel.getPlayerInfo(roomId, userId);
    curPlayer.setScore(curPlayer.getScore() - bet);
    Round round = _roundModel.getRound(roundId);
    round.setPotAmount(round.getPotAmount() + bet);

    if (!_roundModel.updateRound(round))
    {
        LOG_ERROR << "更新底池失败!";
        response["errno"] = 1;
        conn->send(response.dump());
        return ;
    }

    if (!_playerModel.updatePlayerInfo(curPlayer) || !_roundModel.updateRound(round))
    {
        response["errno"] = 1;
        response["errmsg"] = "积分计算失败!";
        conn->send(response.dump());
        return ;
    }

    json notifyJson;
    int nextPlayerId = getNextActionPlayerId(roomId, roundId);
    if (!updateRound(roundId, nextPlayerId))
    {
        LOG_ERROR << "更新游戏轮次失败!";
        response["errno"] = 1;
        conn->send(response.dump());
        return ;
    }
    notifyJson["msgid"] = GAME_NOTIFY;
    notifyJson["msg"] = "玩家id: " + std::to_string(userId) + ", 下注了" + to_string(bet) + "\n \
    下一个行动的玩家id: " + to_string(nextPlayerId);
    notifyJson["currentplayerid"] = nextPlayerId;
    notifyJson["pot"] = round.getPotAmount();

    // 把房间中的玩家状态更新
   std::vector<Player> players = _playerModel.getPlayersInRoom(roomId);
   for (Player &player : players)
   {
        json playerJson;
        playerJson["userid"] = player.getUserId();
        playerJson["status"] = player.getStatus();
        playerJson["seatpos"] = player.getSeatPos();
        playerJson["score"] = player.getScore();
        playerJson["name"] = player.getName();

        notifyJson["players"].push_back(playerJson);
    }

    notify(notifyJson, _roomModel.getRoomPlayerIds(roomId));
}

int ChatService::getNextActionPlayerId(int roomid, int roundid)
{
    int nextPlayerId = -1;
    std::vector<Player> players = _playerModel.getNotFoldPlayers(roomid);
    Round round = _roundModel.getRound(roundid);
    int currentPlayerId = round.getCurrentPlayerId();
    Player curPlayer = _playerModel.getPlayerInfo(roomid, currentPlayerId);
    int pos = curPlayer.getSeatPos();
    // 获取当前玩家座位号
    int currentPlayerSeatPos = pos;

    // 从当前座位号下一位开始找，找到下个行动的玩家
    Room curRoom = _roomModel.getRoomInfo(roomid);
    int count = curRoom.getMaxPlayerCount();
    int maxPlayerCount = curRoom.getMaxPlayerCount();
    
    for (int i=0;i<count-1;i++)
    {
        pos++;
        if (pos > maxPlayerCount) // 下一位超过最大座位号，从1开始找
        {
            pos = 1;
        }
        // 如果当前座位号是3，则找4，5，6，1，2即可
        for (Player &player : players)
        {
            if (player.getSeatPos() == pos)
            {
                LOG_INFO << "下一玩家座位号: " << pos << ", 玩家id: " << player.getUserId();
                nextPlayerId = player.getUserId();
                return nextPlayerId;
            }
        }
    }

    return nextPlayerId;
}

bool ChatService::isGameOver(int roomid)
{
    return _playerModel.getNotFoldPlayers(roomid).size() <= 1;
}

void ChatService::gameOver(const TcpConnectionPtr& conn, int roomId, int roundid)
{
    /*
        通知:  {"msgid","msg"}
    */
   json notifyJson;
   notifyJson["msgid"] = GAME_OVER_NOTIFY;
   notifyJson["msg"] = "游戏结束!";

   std::vector<Player> ps = _playerModel.getNotFoldPlayers(roomId);
   assert(ps.size() == 1);
   Player &winPlayer = ps[0];
   // 结算战绩
   Round round = _roundModel.getRound(roundid);
   winPlayer.setScore(winPlayer.getScore() + round.getPotAmount());
   if (!_playerModel.updatePlayerInfo(winPlayer))
   {
       LOG_ERROR << "结算赢家战绩失败!";
       return ;
   }
   // 把房间中的玩家状态更新
   std::vector<Player> players = _playerModel.getPlayersInRoom(roomId);
   for (Player &player : players)
   {
       player.setState("UNSEEN");
       if (!_playerModel.updatePlayerInfo(player))
        {
            LOG_ERROR << "更新玩家状态失败!";
            return ;
        }
        json playerJson;
        playerJson["userid"] = player.getUserId();
        playerJson["status"] = player.getStatus();
        playerJson["seatpos"] = player.getSeatPos();
        playerJson["score"] = player.getScore();
        playerJson["name"] = player.getName();

        notifyJson["players"].push_back(playerJson);
    }
    notify(notifyJson, _roomModel.getRoomPlayerIds(roomId));
}

bool ChatService::updateRound(int roundid, int nextActionPlayerId)
{
    Round round = _roundModel.getRound(roundid);
    round.setCurrentPlayerId(nextActionPlayerId);
    return _roundModel.updateRound(round);
}