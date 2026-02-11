#include "model/offlinemessagemodel.hpp"
#include "db/db.h"

using namespace std;
// 存储用户的离线消息
void OfflineMsgModel::insert(int userid, string msg)
{
    char sql[1024] = {0};

    sprintf(sql, "insert into offlinemessage(userid,message) values(%d,'%s')",
            userid, msg.c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}
// 删除用户的离线消息
void OfflineMsgModel::remove(int userid)
{
    char sql[1024] = {0};

    sprintf(sql, "delete from offlinemessage where userid=%d",
            userid);

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}
// 查询用户的离线消息
vector<string> OfflineMsgModel::query(int userid)
{
    //组装用户的离线消息
    char sql[1024] = {0};
    sprintf(sql,"select message from offlinemessage where userid = %d",userid);
    vector<string>vec;  //存储离线消息
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)
        {
            //把useid用户的所有离线消息存入vec中返回
            MYSQL_ROW row;  //取一行存一行
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                vec.push_back(row[0]);  //存储第一列的message字段
            }
            mysql_free_result(res);
        }
    }
    return vec;
}