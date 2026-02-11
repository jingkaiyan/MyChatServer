#ifndef USERMODEL_H
#define USERMODEL_H

/*
 * UserModel 负责对 User 表的数据库操作封装。
 * - 对外提供增删改查接口（目前声明了 insert）。
 * - 业务层通过该类访问数据库，避免直接写 SQL 细节。
 */

#include"user.hpp"

class UserModel{
public:
    //User表的增加方法
    bool insert(User &user);
    //根据用户id查询用户信息
    User query(int id);
    //更新用户状态信息
    bool updateState(User user);
    //重置所有在线用户状态为offline（服务重启时使用）
    bool resetState();
private:
};

#endif // !USERMODEL_H