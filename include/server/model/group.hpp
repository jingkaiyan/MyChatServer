#ifndef GROUP_H
#define GROUP_H
/*
 * Group 的 ORM 类：用于群组信息映射。
 * - 保存群组基础信息（id、name、desc）。
 * - 维护群组用户列表（users）。
 */
#include<string>
#include<vector>
#include"groupuser.hpp"

using namespace std;
class Group{
public:
    Group(int id = -1,string name ="",string desc = "")
    {
        this -> id = id;
        this -> name = name;
        this -> desc = desc;    // 群组描述
    }
    void setId(int id){this -> id = id;}
    void setName(string name){this -> name = name;}
    void setDesc(string desc){this -> desc = desc;}
    int getId(){return id;}
    string getName(){return name;}
    string getDesc(){return desc;}
    vector<GroupUser>&getUsers(){return this -> users;}
private:
    int id;         // 群组id
    string name;    // 群组名称
    string desc;    // 群组描述
    vector<GroupUser> users;    // 群组用户列表
};

#endif // !GROUP_H