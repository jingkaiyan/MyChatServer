#include "model/usermodel.hpp"
#include <iostream>
#include <db/db.h>
#include <algorithm>
#include <cctype>

/*
 * UserModel 负责对 User 表的数据库操作封装。
 * - 对外提供增删改查接口（目前声明了 insert）。
 * - 业务层通过该类访问数据库，避免直接写 SQL 细节。
 */

// User表的增加方法
bool UserModel::insert(User &user)
{
    MySQL mysql;
    if (mysql.connect())
    {
        string escapedName = mysql.escapeString(user.getName());
        string escapedPassword = mysql.escapeString(user.getPassword());
        string escapedState = mysql.escapeString(user.getState());
        char sql[1024] = {0};
        snprintf(sql, sizeof(sql), "insert into user(name,password,state) values('%s',sha2('%s',256),'%s')",
             escapedName.c_str(), escapedPassword.c_str(), escapedState.c_str());

        if (mysql.update(sql))
        {
            // 获取插入成功的用户数据生成的主键id
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

bool UserModel::checkPassword(int id, const string &password)
{
    MySQL mysql;
    if (!mysql.connect())
    {
        return false;
    }

    string escapedPassword = mysql.escapeString(password);
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "select password from user where id = %d and (password = sha2('%s',256) or password = '%s') limit 1",
             id,
             escapedPassword.c_str(),
             escapedPassword.c_str());

    MYSQL_RES *res = mysql.query(sql);
    if (res == nullptr)
    {
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    if (row == nullptr || row[0] == nullptr)
    {
        mysql_free_result(res);
        return false;
    }

    string storedPassword = row[0];
    mysql_free_result(res);

    bool looksLikeSha256 = storedPassword.size() == 64 &&
                           all_of(storedPassword.begin(), storedPassword.end(), [](unsigned char ch) {
                               return isxdigit(ch) != 0;
                           });

    if (!looksLikeSha256)
    {
        char upgradeSql[1024] = {0};
        snprintf(upgradeSql, sizeof(upgradeSql),
                 "update user set password = sha2('%s',256) where id = %d and password = '%s'",
                 escapedPassword.c_str(),
                 id,
                 escapedPassword.c_str());
        mysql.update(upgradeSql);
    }

    return true;
}
// 根据用户id查询用户信息
User UserModel::query(int id)
{
    // 组装sql语句
    char sql[1024] = {};
    sprintf(sql, "select * from user where id = %d", id);
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPassword(row[2]);
                user.setState(row[3]);
                mysql_free_result(res);
                return user;
            }
        }
    }
    // 失败返回空对象
    return User();
}

// 更新用户状态信息
bool UserModel::updateState(User user)
{
    MySQL mysql;
    if (mysql.connect())
    {
        string escapedState = mysql.escapeString(user.getState());
        char sql[1024] = {};
        snprintf(sql, sizeof(sql), "update user set state = '%s' where id = %d", escapedState.c_str(), user.getId());
        if(mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}

// 将所有在线用户状态重置为 offline（服务器启动时调用，避免遗留脏数据）
bool UserModel::resetState()
{
    char sql[128] = "update user set state = 'offline' where state = 'online'";
    MySQL mysql;
    if (mysql.connect())
    {
        return mysql.update(sql);
    }
    return false;
}