#include "model/friendmodel.hpp"
#include "db/db.h"

// 添加好友
void FriendModel::insert(int userid, int friendid)
{
    //组装sql语句
    char sql[1024] = {0};
    sprintf(sql,"insert into friend values(%d,%d)",userid,friendid);
    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
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