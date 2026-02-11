#ifndef USER_H
#define USER_H

#include<string>
using namespace std;

/*
 * User 的 ORM 类：用于在程序与数据库用户表之间建立映射。
 * - 保存用户基础信息（id、name、password、state）。
 * - 提供构造与 set/get 接口，便于业务层读写与后续校验扩展。
 */

//User的ORM类：编程语言与数据库之间建立映射关系
class User
{
public:
    User(int id = -1,string name = "",string pwd = "",string state = "offline")
    {
        this -> id = id;
        this -> name = name;
        this -> password = pwd;
        this -> state = state;
    }
    /*设置set和get方法，便于后续加校验和业务规则*/
    //设置set方法
    void setId(int id) { this -> id = id; }
    void setName(string name) { this -> name = name; }
    void setPassword(string pwd) { this -> password = pwd; }
    void setState(string state) { this -> state = state; }

    //设置get方法
    int getId() { return this -> id; }
    string getName() { return this -> name; }
    string getPassword() { return this -> password; }
    string getState() { return this -> state; }
private:
    int id;            //用户id
    string name;      //用户名
    string password;  //用户密码
    string state;     //用户状态
};

#endif // !USER_H