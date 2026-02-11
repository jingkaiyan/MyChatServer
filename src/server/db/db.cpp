#include "db/db.h"
#include <muduo/base/Logging.h>
#include <string>
/*
 * 数据库操作实现：
 * - 封装 MySQL 连接的初始化与释放，统一管理 _conn。
 * - connect() 建立连接并设置字符集（utf8）。
 * - update() 执行写操作，失败记录日志并返回 false。
 * - query() 执行读操作，失败记录日志并返回 nullptr。
 */
using namespace std;
// 初始化相关信息
string server = "127.0.0.1";
string user = "root";
string password = "123456";
string dbname = "chat";

// 初始化数据库连接
MySQL::MySQL()
{
    _conn = mysql_init(nullptr);
}
// 释放数据库连接
MySQL::~MySQL()
{
    if (_conn != nullptr)
    {
        mysql_close(_conn);
    }
}
// 连接数据库
bool MySQL::connect()
{
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
                                  password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    if (p != nullptr)
    {
        mysql_query(_conn, "set names utf8"); // 设置utf-8字符集
        LOG_INFO << "[数据库] 连接成功";
    }
    else
    {
        LOG_INFO << "[数据库] 连接失败，错误: " << mysql_error(_conn);
    }
    return p;
}
// 更新操作
bool MySQL::update(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << "[数据库] 更新失败 " << __FILE__ << ":" << __LINE__
                 << " SQL=" << sql << " 错误: " << mysql_error(_conn);
        return false;
    }
    return true;
}
// 查询操作
MYSQL_RES *MySQL::query(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << "[数据库] 查询失败 " << __FILE__ << ":" << __LINE__
                 << " SQL=" << sql;
        return nullptr;
    }
    return mysql_use_result(_conn);
}
// 获取连接
MYSQL *MySQL::getConnection()
{
    return _conn;
}