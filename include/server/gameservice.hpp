#ifndef GAMESERVICE_H
#define GAMESERVICE_H

#include <vector>
#include "public.hpp"


class GameService
{
public:
    static GameService& getInstance();
    // 初始化全部牌
    void initCards();
    // 随机发牌三张
    std::vector<Card> getRandomCards();
private:
    GameService() = default;
    ~GameService() = default;
    GameService(const GameService&) = delete;
private:
    std::vector<Card> cards;
    static GameService instance;
};

#endif