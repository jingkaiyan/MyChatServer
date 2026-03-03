#ifndef GROUPMODEL_H
#define GROUPMODEL_H
/*
 * GroupModel：群组表的数据库访问封装。
 * - 创建群组、添加成员。
 * - 查询用户所在群组与群成员列表。
 */
#include "group.hpp"
#include "groupuser.hpp"

class GroupModel{
public:
    //创建群组
    bool createGroup(Group &group); //使用引用便于修改传入的group对象
    //加入群组
    bool addGroup(int userid,int groupid,string role);
    //查询用户所在的群组信息
    vector<Group> queryGroups(int userid);
    //根据指定的groupid查询用户的id列表，除了userid自己，主要用户群聊业务给群组其他成员发消息
    vector<int> queryGroupusers(int userid,int groupid);
};

#endif // !GROUPMODEL_H