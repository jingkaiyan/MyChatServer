#ifndef PUBLIC_H
#define PUBLIC_H
/*
 * 公共消息类型定义：
 * - 为客户端/服务器约定的业务消息编号，便于双方统一解析与分发。
 * - LOGIN_MSG：登录请求
 * - REG_MSG：注册请求
 */
enum EnMsgType
{
    LOGIN_MSG =1,       //登录消息
    REG_MSG,            //注册消息
    REG_MSG_ACK,        //注册响应消息
    LOG_MSG_ACK,        //登录响应消息
    LOGINOUT_MSG,       //注销消息
    ONE_CHAT_MSG,       //一对一聊天消息
    ADD_FRIEND_MSG,     //添加好友消息
    ADD_FRIEND_MSG_ACK, //添加好友响应消息
    CREATE_GROUP_MSG,   //创建群组消息
    CREATE_GROUP_MSG_ACK, //创建群组响应消息
    ADD_GROUP_MSG,      //加入群组消息
    ADD_GROUP_MSG_ACK,  //加入群组响应消息
    GROUP_CHAT_MSG,     //群聊消息
    FRIEND_STATE_MSG,   //好友状态变更消息
};
#endif // !PUBLIC_H