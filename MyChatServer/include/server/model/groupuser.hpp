#ifndef GROUPUSER_H
#define GROUPUSER_H
/*
 * GroupUser 的 ORM 类：用于群组成员信息映射。
 * - 继承 User，复用用户基础信息。
 * - 额外维护群组角色（role）。
 */
#include "user.hpp"

class GroupUser : public User{
public:
    void setRole(string role){this -> role = role;}
    string getRole(){return this -> role;}
private:
    string role;
};
#endif // !GROUPUSER_H