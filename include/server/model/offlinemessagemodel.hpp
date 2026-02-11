#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H
/*
 * OfflineMsgModel：离线消息表的数据库访问封装。
 * - insert(userid, msg)：写入用户的离线消息。
 * - remove(userid)：用户上线后删除其离线消息。
 * - query(userid)：查询并返回该用户的所有离线消息（按插入顺序）。
 */
#include<string>
#include<vector>
using namespace std;

class OfflineMsgModel{
public:
    //存储用户的离线消息
    void insert(int userid,string msg);
    //删除用户的离线消息
    void remove(int userid);
    //查询用户的离线消息
    vector<string> query(int userid);
};
#endif