/*
* 
*/

#ifndef DB_H
#define DB_H

#include<string>
#include<mysql/mysql.h>

using namespace std;
//数据库操作类
class MySQL
{
public:
    //初始化数据库连接
    MySQL();
    //释放数据库连接
    ~MySQL();
    //连接数据库
    bool connect();
    //更新操作
    bool update(string sql);
    //查询操作
    MYSQL_RES* query(string sql);
    //SQL 字符串转义（不包含两侧引号）
    string escapeString(const string &value);
    //获取连接
    MYSQL* getConnection();
private:
    MYSQL* _conn; //数据库连接句柄
};

#endif // !DB_H
