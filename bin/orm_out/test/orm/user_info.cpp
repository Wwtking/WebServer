#include "user_info.h"
#include "log.h"
#include "json_util.h"

namespace test {
namespace orm {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("orm");

UserInfo::UserInfo()
    :m_status(10)
    ,m_id()
    ,m_name()
    ,m_email("xx@xx.com")
    ,m_phone()
    ,m_createTime()
    ,m_updateTime(time(0)) {
}

std::string UserInfo::toJsonString() const {
    Json::Value jvalue;
    jvalue["id"] = std::to_string(m_id);
    jvalue["name"] = m_name;
    jvalue["email"] = m_email;
    jvalue["phone"] = m_phone;
    jvalue["status"] = m_status;
    jvalue["create_time"] = sylar::TimeToStr(m_createTime);
    jvalue["update_time"] = sylar::TimeToStr(m_updateTime);
    return sylar::JsonUtil::ToString(jvalue);
}

void UserInfo::setId(const int64_t& v) {
    m_id = v;
}

void UserInfo::setName(const std::string& v) {
    m_name = v;
}

void UserInfo::setEmail(const std::string& v) {
    m_email = v;
}

void UserInfo::setPhone(const std::string& v) {
    m_phone = v;
}

void UserInfo::setStatus(const int32_t& v) {
    m_status = v;
}

void UserInfo::setCreateTime(const int64_t& v) {
    m_createTime = v;
}

void UserInfo::setUpdateTime(const int64_t& v) {
    m_updateTime = v;
}


int UserInfoDao::Update(UserInfo::ptr info, sylar::IDB::ptr conn) {
    std::string sql = "update user set name = ?, email = ?, phone = ?, status = ?, create_time = ?, update_time = ? where id = ?";
    auto stmt = conn->prepare(sql);
    if(!stmt) {
        SYLAR_LOG_ERROR(g_logger) << "stmt=" << sql
                 << " errno=" << conn->getErrno() << " errstr=" << conn->getErrStr();
        return conn->getErrno();
    }
    stmt->bindString(1, info->m_name);
    stmt->bindString(2, info->m_email);
    stmt->bindString(3, info->m_phone);
    stmt->bindInt32(4, info->m_status);
    stmt->bindTime(5, info->m_createTime);
    stmt->bindTime(6, info->m_updateTime);
    stmt->bindInt64(7, info->m_id);
    return stmt->execute();
}

int UserInfoDao::Insert(UserInfo::ptr info, sylar::IDB::ptr conn) {
    std::string sql = "insert into user (name, email, phone, status, create_time, update_time) values (?, ?, ?, ?, ?, ?)";
    auto stmt = conn->prepare(sql);
    if(!stmt) {
        SYLAR_LOG_ERROR(g_logger) << "stmt=" << sql
                 << " errno=" << conn->getErrno() << " errstr=" << conn->getErrStr();
        return conn->getErrno();
    }
    stmt->bindString(1, info->m_name);
    stmt->bindString(2, info->m_email);
    stmt->bindString(3, info->m_phone);
    stmt->bindInt32(4, info->m_status);
    stmt->bindTime(5, info->m_createTime);
    stmt->bindTime(6, info->m_updateTime);
    int rt = stmt->execute();
    if(rt == 0) {
        info->m_id = conn->getLastInsertId();
    }
    return rt;
}

int UserInfoDao::InsertOrUpdate(UserInfo::ptr info, sylar::IDB::ptr conn) {
    if(info->m_id == 0) {
        return Insert(info, conn);
    }
    std::string sql = "replace into user (id, name, email, phone, status, create_time, update_time) values (?, ?, ?, ?, ?, ?, ?)";
    auto stmt = conn->prepare(sql);
    if(!stmt) {
        SYLAR_LOG_ERROR(g_logger) << "stmt=" << sql
                 << " errno=" << conn->getErrno() << " errstr=" << conn->getErrStr();
        return conn->getErrno();
    }
    stmt->bindInt64(1, info->m_id);
    stmt->bindString(2, info->m_name);
    stmt->bindString(3, info->m_email);
    stmt->bindString(4, info->m_phone);
    stmt->bindInt32(5, info->m_status);
    stmt->bindTime(6, info->m_createTime);
    stmt->bindTime(7, info->m_updateTime);
    return stmt->execute();
}

int UserInfoDao::Delete(UserInfo::ptr info, sylar::IDB::ptr conn) {
    std::string sql = "delete from user where id = ?";
    auto stmt = conn->prepare(sql);
    if(!stmt) {
        SYLAR_LOG_ERROR(g_logger) << "stmt=" << sql
                 << " errno=" << conn->getErrno() << " errstr=" << conn->getErrStr();
        return conn->getErrno();
    }
    stmt->bindInt64(1, info->m_id);
    return stmt->execute();
}

int UserInfoDao::DeleteById( const int64_t& id, sylar::IDB::ptr conn) {
    std::string sql = "delete from user where id = ?";
    auto stmt = conn->prepare(sql);
    if(!stmt) {
        SYLAR_LOG_ERROR(g_logger) << "stmt=" << sql
                 << " errno=" << conn->getErrno() << " errstr=" << conn->getErrStr();
        return conn->getErrno();
    }
    stmt->bindInt64(1, id);
    return stmt->execute();
}

int UserInfoDao::DeleteByName( const std::string& name, sylar::IDB::ptr conn) {
    std::string sql = "delete from user where name = ?";
    auto stmt = conn->prepare(sql);
    if(!stmt) {
        SYLAR_LOG_ERROR(g_logger) << "stmt=" << sql
                 << " errno=" << conn->getErrno() << " errstr=" << conn->getErrStr();
        return conn->getErrno();
    }
    stmt->bindString(1, name);
    return stmt->execute();
}

int UserInfoDao::DeleteByEmail( const std::string& email, sylar::IDB::ptr conn) {
    std::string sql = "delete from user where email = ?";
    auto stmt = conn->prepare(sql);
    if(!stmt) {
        SYLAR_LOG_ERROR(g_logger) << "stmt=" << sql
                 << " errno=" << conn->getErrno() << " errstr=" << conn->getErrStr();
        return conn->getErrno();
    }
    stmt->bindString(1, email);
    return stmt->execute();
}

int UserInfoDao::DeleteByStatus( const int32_t& status, sylar::IDB::ptr conn) {
    std::string sql = "delete from user where status = ?";
    auto stmt = conn->prepare(sql);
    if(!stmt) {
        SYLAR_LOG_ERROR(g_logger) << "stmt=" << sql
                 << " errno=" << conn->getErrno() << " errstr=" << conn->getErrStr();
        return conn->getErrno();
    }
    stmt->bindInt32(1, status);
    return stmt->execute();
}

int UserInfoDao::QueryAll(std::vector<UserInfo::ptr>& results, sylar::IDB::ptr conn) {
    std::string sql = "select id, name, email, phone, status, create_time, update_time from user";
    auto stmt = conn->prepare(sql);
    if(!stmt) {
        SYLAR_LOG_ERROR(g_logger) << "stmt=" << sql
                 << " errno=" << conn->getErrno() << " errstr=" << conn->getErrStr();
        return conn->getErrno();
    }
    auto rt = stmt->query();
    if(!rt) {
        return stmt->getErrno();
    }
    while (rt->next()) {
        UserInfo::ptr v(new UserInfo);
        v->m_id = rt->getInt64(0);
        v->m_name = rt->getString(1);
        v->m_email = rt->getString(2);
        v->m_phone = rt->getString(3);
        v->m_status = rt->getInt32(4);
        v->m_createTime = rt->getTime(5);
        v->m_updateTime = rt->getTime(6);
        results.push_back(v);
    }
    return 0;
}

UserInfo::ptr UserInfoDao::Query( const int64_t& id, sylar::IDB::ptr conn) {
    std::string sql = "select id, name, email, phone, status, create_time, update_time from user where id = ?";
    auto stmt = conn->prepare(sql);
    if(!stmt) {
        SYLAR_LOG_ERROR(g_logger) << "stmt=" << sql
                 << " errno=" << conn->getErrno() << " errstr=" << conn->getErrStr();
        return nullptr;
    }
    stmt->bindInt64(1, id);
    auto rt = stmt->query();
    if(!rt) {
        return nullptr;
    }
    if(!rt->next()) {
        return nullptr;
    }
    UserInfo::ptr v(new UserInfo);
    v->m_id = rt->getInt64(0);
    v->m_name = rt->getString(1);
    v->m_email = rt->getString(2);
    v->m_phone = rt->getString(3);
    v->m_status = rt->getInt32(4);
    v->m_createTime = rt->getTime(5);
    v->m_updateTime = rt->getTime(6);
    return v;
}

UserInfo::ptr UserInfoDao::QueryByName( const std::string& name, sylar::IDB::ptr conn) {
    std::string sql = "select id, name, email, phone, status, create_time, update_time from user where name = ?";
    auto stmt = conn->prepare(sql);
    if(!stmt) {
        SYLAR_LOG_ERROR(g_logger) << "stmt=" << sql
                 << " errno=" << conn->getErrno() << " errstr=" << conn->getErrStr();
        return nullptr;
    }
    stmt->bindString(1, name);
    auto rt = stmt->query();
    if(!rt) {
        return nullptr;
    }
    if(!rt->next()) {
        return nullptr;
    }
    UserInfo::ptr v(new UserInfo);
    v->m_id = rt->getInt64(0);
    v->m_name = rt->getString(1);
    v->m_email = rt->getString(2);
    v->m_phone = rt->getString(3);
    v->m_status = rt->getInt32(4);
    v->m_createTime = rt->getTime(5);
    v->m_updateTime = rt->getTime(6);
    return v;
}

UserInfo::ptr UserInfoDao::QueryByEmail( const std::string& email, sylar::IDB::ptr conn) {
    std::string sql = "select id, name, email, phone, status, create_time, update_time from user where email = ?";
    auto stmt = conn->prepare(sql);
    if(!stmt) {
        SYLAR_LOG_ERROR(g_logger) << "stmt=" << sql
                 << " errno=" << conn->getErrno() << " errstr=" << conn->getErrStr();
        return nullptr;
    }
    stmt->bindString(1, email);
    auto rt = stmt->query();
    if(!rt) {
        return nullptr;
    }
    if(!rt->next()) {
        return nullptr;
    }
    UserInfo::ptr v(new UserInfo);
    v->m_id = rt->getInt64(0);
    v->m_name = rt->getString(1);
    v->m_email = rt->getString(2);
    v->m_phone = rt->getString(3);
    v->m_status = rt->getInt32(4);
    v->m_createTime = rt->getTime(5);
    v->m_updateTime = rt->getTime(6);
    return v;
}

int UserInfoDao::QueryByStatus(std::vector<UserInfo::ptr>& results,  const int32_t& status, sylar::IDB::ptr conn) {
    std::string sql = "select id, name, email, phone, status, create_time, update_time from user where status = ?";
    auto stmt = conn->prepare(sql);
    if(!stmt) {
        SYLAR_LOG_ERROR(g_logger) << "stmt=" << sql
                 << " errno=" << conn->getErrno() << " errstr=" << conn->getErrStr();
        return conn->getErrno();
    }
    stmt->bindInt32(1, status);
    auto rt = stmt->query();
    if(!rt) {
        return 0;
    }
    while (rt->next()) {
        UserInfo::ptr v(new UserInfo);
        v->m_id = rt->getInt64(0);
        v->m_name = rt->getString(1);
        v->m_email = rt->getString(2);
        v->m_phone = rt->getString(3);
        v->m_status = rt->getInt32(4);
        v->m_createTime = rt->getTime(5);
        v->m_updateTime = rt->getTime(6);
        results.push_back(v);
    };
    return 0;
}

int UserInfoDao::CreateTableSQLite3(sylar::IDB::ptr conn) {
    return conn->execute("CREATE TABLE user("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name TEXT NOT NULL DEFAULT '',"
            "email TEXT NOT NULL DEFAULT 'xx@xx.com',"
            "phone TEXT NOT NULL DEFAULT '',"
            "status INTEGER NOT NULL DEFAULT 10,"
            "create_time TIMESTAMP NOT NULL DEFAULT '1980-01-01 00:00:00',"
            "update_time TIMESTAMP NOT NULL DEFAULT current_timestamp);"
            "CREATE UNIQUE INDEX user_name ON user(name);"
            "CREATE UNIQUE INDEX user_email ON user(email);"
            "CREATE INDEX user_status ON user(status);"
            );
}

int UserInfoDao::CreateTableMySQL(sylar::IDB::ptr conn) {
    return conn->execute("CREATE TABLE user("
            "`id` bigint AUTO_INCREMENT COMMENT '唯一主键',"
            "`name` varchar(30) NOT NULL DEFAULT '' COMMENT '名称',"
            "`email` varchar(128) NOT NULL DEFAULT 'xx@xx.com',"
            "`phone` varchar(128) NOT NULL DEFAULT '',"
            "`status` int NOT NULL DEFAULT 10,"
            "`create_time` timestamp NOT NULL DEFAULT '1980-01-01 00:00:00',"
            "`update_time` timestamp NOT NULL DEFAULT current_timestamp ON UPDATE current_timestamp ,"
            "PRIMARY KEY(`id`),"
            "UNIQUE KEY `user_name` (`name`),"
            "UNIQUE KEY `user_email` (`email`),"
            "KEY `user_status` (`status`))");
}
} //namespace orm
} //namespace test
