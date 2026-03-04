#include "model/friendmodel.hpp"
#include "db/db.h"

// 添加好友
bool FriendModel::insert(int userid, int friendid)
{
    // 组装sql语句（双向关系）
    char sql1[1024] = {0};
    char sql2[1024] = {0};
    sprintf(sql1, "insert ignore into friend values(%d,%d)", userid, friendid);
    sprintf(sql2, "insert ignore into friend values(%d,%d)", friendid, userid);
    MySQL mysql;
    if (mysql.connect())
    {
        bool ok1 = mysql.update(sql1);
        bool ok2 = mysql.update(sql2);
        return ok1 && ok2;
    }
    return false;
}

// 删除好友
bool FriendModel::remove(int userid, int friendid)
{
    //组装sql语句，双向删除
    char sql[1024] = {0};
    sprintf(sql,"delete from friend where (userid=%d and friendid=%d) or (userid=%d and friendid=%d)",
            userid, friendid, friendid, userid);
    MySQL mysql;
    if(mysql.connect())
    {
        return mysql.update(sql);
    }
    return false;
}

// 返回用户的好友列表
vector<User> FriendModel::query(int userid)
{
    //组装sql语句
    char sql[1024] = {0};
    //user表和friend表进行夺标联合查询
    sprintf(sql,"select a.id,a.name,a.state from user a inner join friend b on b.friendid = a.id where b.userid = %d",userid);
    vector<User> vec;   //存储用户
    MySQL mysql;    
    if(mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr)
        {
            //把userid用户的所有好友信息，填充到vec中
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));   
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
} 