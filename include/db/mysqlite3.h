#ifndef __SYLAR_SQLITE3_H__
#define __SYLAR_SQLITE3_H__

#include <sqlite3.h>
#include <memory>
#include <string>
#include <list>
#include <map>
#include "noncopyable.h"
#include "db.h"
#include "thread.h"
#include "singleton.h"

namespace sylar {

class SQLite3Stmt;

namespace {
template<size_t N, typename... Args>
struct SQLite3Binder {
    static int Bind(std::shared_ptr<SQLite3Stmt> stmt) {
        return SQLITE_OK;
    }
};
}

// 使用SQLite数据库API执行SQL语句的步骤：
// 一、使用 sqlite3_stmt 执行
//    1. 先使用 sqlite3_prepare_v2 函数将字符串SQL语句编译为一个 sqlite3_stmt 对象
//    2. 再使用 sqlite3_bind 对 sqlite3_stmt 对象进行参数绑定(如果1中的SQL语句是格式化的)
//    3. 然后再使用 sqlite3_step 函数执行该对象，用于执行查询并返回下一行数据
//    4. 最后在不需要 sqlite3_stmt 对象时，需使用 sqlite3_finalize 函数将其销毁以释放内存和资源
// 二、使用 sqlite3_exec 执行
//    相当于是把 sqlite3_prepare_v2、sqlite3_step、sqlite3_finalize 结合在一起
//    每次执行都需要经过这三个过程，在执行“语句结构不变,只改参数”的sql语句中效率低

class SQLite3Manager;
/**
 * @brief 数据库连接对象封装
*/
class SQLite3 : public IDB, public std::enable_shared_from_this<SQLite3> {
friend class SQLite3Manager;
public:
    typedef std::shared_ptr<SQLite3> ptr;

    // 打开数据库的方式
    enum Flags {
        READONLY = SQLITE_OPEN_READONLY,    // 只读
        READWRITE = SQLITE_OPEN_READWRITE,  // 读写
        CREATE = SQLITE_OPEN_CREATE         // 创建
    };

    /**
     * @brief 创建数据库
    */
    static SQLite3::ptr Create(sqlite3* db);

    /**
     * @brief 打开/创建数据库
     * @details 如果数据库存在，以flags方式打开，如果不存在则创建
     * @param[in] dbname 数据库名称
     * @param[in] flags 选项
    */
    static SQLite3::ptr Create(const std::string& dbname, int flags = READWRITE | CREATE);

    /**
     * @brief 析构函数，关闭数据库
    */
    ~SQLite3();

    /**
     * @brief 编译字符串sql语句，生成可执行的sql语句实例对象(sqlite3_stmt对象)
     * @param[in] stmt 字符串形式sql语句
    */
    IStmt::ptr prepare(const std::string& stmt) override;

    /**
     * @brief 用于获取最近一次数据库操作的错误码
    */
    int getErrno() override;

    /**
     * @brief 用于获取最近一次数据库操作的错误信息
    */
    std::string getErrStr() override;
    
    /**
     * @brief 格式化执行一条或多条SQL语句
     * @details 将格式化的字符串和参数列表转换为SQL语句，再去执行
     * @param[in] format SQL语句字符串格式
     * @param[in] ... 可变参数列表
     * @example execute("INSERT INTO mytable (name, age) VALUES ('%s', %d);", name, age);
    */
    int execute(const char* format, ...) override;
    int execute(const char* format, va_list ap);

    /**
     * @brief 用于执行一条或多条SQL语句
     * @param[in] sql 要执行的SQL语句的字符串表示
    */
    int execute(const std::string& sql) override;

    /**
     * @brief 用于获取最近插入行的行ID
    */
    int64_t getLastInsertId() override;

    /**
     * @brief 格式化获取数据库查询结果
     * @param[in] format SQL查询语句字符串格式
     * @param[in] ... 可变参数列表
    */
    ISQLData::ptr query(const char* format, ...) override;

    /**
     * @brief 获取数据库查询结果
     * @param[in] sql 要执行的SQL查询语句的字符串表示
    */
    ISQLData::ptr query(const std::string& sql) override;

    /**
     * @brief 开启事务
    */
    ITransaction::ptr openTransaction(bool auto_commit = false) override;

    /**
     * @brief 格式化执行SQL语句
     * @details 封装了prepare、bind、step三个函数
     * @param[in] stmt 要执行的SQL语句的字符串表示
    */
    template<typename... Args>
    int execStmt(const char* stmt, Args&&... args);

    /**
     * @brief 格式化获取数据库查询结果
     * @details 封装了prepare、bind、query三个函数
     * @param[in] stmt 要执行的SQL语句的字符串表示
    */
    template<class... Args>
    ISQLData::ptr queryStmt(const char* stmt, const Args&... args);

    /**
     * @brief 关闭数据库
    */
    int close();

    /**
     * @brief 获取数据库
    */
    sqlite3* getDB() const { return m_db;}

private:
    /**
     * @brief 构造函数
    */
    SQLite3(sqlite3* db);

private:
    sqlite3* m_db;                  // 数据库
    uint64_t m_lastUsedTime = 0;    // 最近一次使用时间
};


class SQLite3Stmt;
/**
 * @brief 表示SQLite数据库查询结果，提供了对查询结果的访问和操作方法
*/
class SQLite3Data : public ISQLData {
public:
    typedef std::shared_ptr<SQLite3Data> ptr;
    
    /**
     * @brief 构造函数
    */
    SQLite3Data(std::shared_ptr<SQLite3Stmt> stmt, int err, const char* errstr);

    /**
     * @brief 获取当前查询结果中的列数
    */
    int getDataCount() override;

    /**
     * @brief 获取整个查询结果集中的列数
    */
    int getColumnCount() override;

    /**
     * @brief 获取查询结果中指定列的数据长度
     * @param[in] idx 指定列的索引
    */
    int getColumnBytes(int idx);

    /**
     * @brief 获取查询结果中指定列的数据类型
     * @param[in] idx 指定列的索引
    */
    int getColumnType(int idx);

    /**
     * @brief 获取查询结果中指定列的列名
     * @param[in] idx 指定列的索引
    */
    std::string getColumnName(int idx);

    /**
     * @brief 获取查询结果中指定列的整数值int/小数double/字符串text/二进制blob
     * @param[in] idx 是指定列的索引
    */
    bool isNull(int idx) override;
    int8_t getInt8(int idx) override;
    uint8_t getUint8(int idx) override;
    int16_t getInt16(int idx) override;
    uint16_t getUint16(int idx) override;
    int32_t getInt32(int idx) override;
    uint32_t getUint32(int idx) override;
    int64_t getInt64(int idx) override;
    uint64_t getUint64(int idx) override;
    float getFloat(int idx) override;
    double getDouble(int idx) override;
    std::string getString(int idx) override;
    std::string getBlob(int idx) override;
    // 获取字符串格式的时间
    time_t getTime(int idx) override;

    /**
     * @brief 执行sql语句
    */
    bool next();

    /**
     * @brief 获取错误码
    */
    int getErrno() const override { 
        return m_errno; 
    }

    /**
     * @brief 获取错误信息
    */
    const std::string& getErrStr() const override { 
        return m_errstr; 
    }

private:
    int m_errno;            // 错误码
    bool m_first;
    std::string m_errstr;   // 错误信息
    std::shared_ptr<SQLite3Stmt> m_stmt;    // SQL语句对象
};


/**
 * @brief SQL语句对象封装
 * @details 用于表示SQLite3数据库预编译语句，存储和操作已经编译的SQL语句
 *      SQLite3Stmt对象通常由sqlite3_prepare_v2函数创建，并在执行SQL语句之前进行绑定和设置
 *      这个类提供了对预编译语句的访问和操作方法，使得执行SQL操作更加方便和高效
*/
class SQLite3Stmt : public IStmt, public std::enable_shared_from_this<SQLite3Stmt> {
friend class SQLite3Data;
public:
    typedef std::shared_ptr<SQLite3Stmt> ptr;

    enum Type {
        COPY = 1,
        REF = 2
    };

    /**
     * @brief 创建SQLite3Stmt对象，并生成可执行的sql语句实例
     * @param[in] db 数据库名称
     * @param[in] stmt 字符串形式sql语句
    */
    static SQLite3Stmt::ptr Create(SQLite3::ptr db, const char* stmt);

    /**
     * @brief 析构函数，释放sqlite3_stmt对象
    */
    ~SQLite3Stmt();

    /**
     * @brief sql语句的准备，生成可执行的sql语句实例对象(sqlite3_stmt对象)
     * @param[in] stmt 字符串形式sql语句
    */
    int prepare(const char* stmt);

    /**
     * @brief 释放由sqlite3_prepare_v2函数编译的SQL语句对象，从内存中删除以避免内存泄漏
    */
    int finish();

    /**
     * @brief 根据参数索引idx绑定value值到预编译的SQL语句中的参数上
     * @param[in] idx 占位符的索引
     * @param[in] idx 要绑定的值
     * @param[in] type 绑定参数的方式
    */
    int bind(int idx, int32_t value);
    int bind(int idx, uint32_t value);
    int bind(int idx, double value);
    int bind(int idx, int64_t value);
    int bind(int idx, uint64_t value);
    int bind(int idx, const char* value, Type type = COPY);
    int bind(int idx, const void* value, int len, Type type = COPY);
    int bind(int idx, const std::string& value, Type type = COPY);
    // 当字符串为NULL时，使用 sqlite3_bind_null 绑定
    // for null type
    int bind(int idx);

    int bindInt8(int idx, const int8_t& value) override;
    int bindUint8(int idx, const uint8_t& value) override;
    int bindInt16(int idx, const int16_t& value) override;
    int bindUint16(int idx, const uint16_t& value) override;
    int bindInt32(int idx, const int32_t& value) override;
    int bindUint32(int idx, const uint32_t& value) override;
    int bindInt64(int idx, const int64_t& value) override;
    int bindUint64(int idx, const uint64_t& value) override;
    int bindFloat(int idx, const float& value) override;
    int bindDouble(int idx, const double& value) override;
    int bindString(int idx, const char* value) override;
    int bindString(int idx, const std::string& value) override;
    // 绑定二进制变量
    int bindBlob(int idx, const void* value, int64_t size) override;
    int bindBlob(int idx, const std::string& value) override;
    // 绑定字符串格式的时间
    int bindTime(int idx, const time_t& value) override;
    int bindNull(int idx) override;

    /**
     * @brief 根据参数索引idx绑定value值到预编译的SQL语句中的参数上
     * @param[in] idx 占位符的索引
     * @param[in] idx 要绑定的值
     * @param[in] type 绑定参数的方式
    */
    int bind(const char* name, int32_t value);
    int bind(const char* name, uint32_t value);
    int bind(const char* name, double value);
    int bind(const char* name, int64_t value);
    int bind(const char* name, uint64_t value);
    int bind(const char* name, const char* value, Type type = COPY);
    int bind(const char* name, const void* value, int len, Type type = COPY);
    int bind(const char* name, const std::string& value, Type type = COPY);
    // for null type
    int bind(const char* name);

    /**
     * @brief 执行编译好的SQL语句对象m_stmt(执行sql语句)
     * @details sqlite3_step 和 sqlite3_exec 函数的区别：
     * sqlite3_step 在执行之前需要调用sqlite3_perpare，之后需要sqlite3_finalize
     * sqlite3_exec 就是把这三个函数结合在了一起，同时提供一个回调函数进行结果的处理
    */
    int step();

    /**
     * @brief 复位SQL语句对象m_stmt，以便下一轮的参数的绑定
    */
    int reset();

    /**
     * @brief 获取ISQLData对象，用于查询
    */
    ISQLData::ptr query() override;

    /**
     * @brief 执行sql语句
    */
    int execute() override;

    /**
     * @brief 用于获取最近插入行的行ID
    */
    int64_t getLastInsertId() override;

    /**
     * @brief 用于获取最近一次数据库操作的错误码
    */
    int getErrno() override;

    /**
     * @brief 用于获取最近一次数据库操作的错误信息
    */
    std::string getErrStr() override;

protected:
    /**
     * @brief 构造函数
    */
    SQLite3Stmt(SQLite3::ptr db);

protected:
    SQLite3::ptr m_db;      // 数据库
    sqlite3_stmt* m_stmt;   // 代表一个编译好的SQL语句，可以用于执行查询和更新操作
};


/**
 * @brief SQL事务操作封装
*/
class SQLite3Transaction : public ITransaction {
public:
    // 事务类型
    enum Type {
        DEFERRED = 0,
        IMMEDIATE = 1,
        EXCLUSIVE = 2
    };

    /**
     * @brief 构造函数
     * @param[in] db 数据库
     * @param[in] auto_commit 是否自动提交
     * @param[in] type 事务类型
    */
    SQLite3Transaction(SQLite3::ptr db
                       ,bool auto_commit = false 
                       ,Type type = DEFERRED);

    /**
     * @brief 析构函数
    */
    ~SQLite3Transaction();

    /**
     * @brief 开启事务
    */
    bool begin() override;

    /**
     * @brief 提交事务
    */
    bool commit() override;

    /**
     * @brief 回滚事务
    */
    bool rollback() override;

    /**
     * @brief 格式化执行一条或多条SQL语句
     * @details 将格式化的字符串和参数列表转换为SQL语句，再去执行
     * @param[in] format SQL语句字符串格式
     * @param[in] ... 可变参数列表
     * @example execute("INSERT INTO mytable (name, age) VALUES ('%s', %d);", name, age);
    */
    int execute(const char* format, ...) override;

    /**
     * @brief 用于执行一条或多条SQL语句
     * @param[in] sql 要执行的SQL语句的字符串表示
    */
    int execute(const std::string& sql) override;

    /**
     * @brief 用于获取最近插入行的行ID
    */
    int64_t getLastInsertId() override;

private:
    SQLite3::ptr m_db;      // 数据库
    Type m_type;            // 事务类型
    int8_t m_status;        // 状态码: 初始0 开启1 提交2 回滚3
    bool m_autoCommit;      // 是否自动提交
};


/**
 * @brief SQL数据库管理
*/
class SQLite3Manager {
public:
    typedef sylar::Mutex MutexType;

    /**
     * @brief 构造函数
    */
    SQLite3Manager();

    /**
     * @brief 析构函数
    */
    ~SQLite3Manager();

    /**
     * @brief 获取数据库连接对象
     * @details 若存在则直接返回，若不存在则根据参数创建
     * @param[in] name 数据库名称
    */
    SQLite3::ptr get(const std::string& name);

    /**
     * @brief 设置数据库创建参数
     * @details 如数据库创建路径path、要执行的sql语句等
    */
    void registerSQLite3(const std::string& name, const std::map<std::string, std::string>& params);

    /**
     * @brief 更新连接池m_conns，存活时间超过sec则删除
    */
    void checkConnection(int sec = 30);

    /**
     * @brief 获取最大连接数
    */
    uint32_t getMaxConn() const { return m_maxConn;}

    /**
     * @brief 设置最大连接数
    */
    void setMaxConn(uint32_t v) { m_maxConn = v;}

    /**
     * @brief 让指定数据库去格式化执行一条或多条SQL语句
     * @details 将格式化的字符串和参数列表转换为SQL语句，再去执行
     * @param[in] name 指定数据库
     * @param[in] format SQL语句字符串格式
     * @param[in] ... 可变参数列表
     * @example execute("user", "INSERT INTO mytable (name, age) VALUES ('%s', %d);", name, age);
    */
    int execute(const std::string& name, const char* format, ...);
    int execute(const std::string& name, const char* format, va_list ap);

    /**
     * @brief 让指定数据库去执行一条或多条SQL语句
     * @param[in] name 指定数据库
     * @param[in] sql 要执行的SQL语句的字符串表示
    */
    int execute(const std::string& name, const std::string& sql);

    /**
     * @brief 让指定数据库去格式化获取数据库查询结果
     * @param[in] name 指定数据库
     * @param[in] format SQL查询语句字符串格式
     * @param[in] ... 可变参数列表
    */
    ISQLData::ptr query(const std::string& name, const char* format, ...);
    ISQLData::ptr query(const std::string& name, const char* format, va_list ap); 

    /**
     * @brief 让指定数据库去获取数据库查询结果
     * @param[in] name 指定数据库
     * @param[in] sql 要执行的SQL查询语句的字符串表示
    */
    ISQLData::ptr query(const std::string& name, const std::string& sql);

    SQLite3Transaction::ptr openTransaction(const std::string& name, bool auto_commit);

private:
    /**
     * @brief 释放数据库连接对象
     * @details 若连接池没满，则放入连接池中，可供后续直接使用(避免重复创建)
     *          若连接池已满，则直接删除
    */
    void freeSQLite3(const std::string& name, SQLite3* m);

private:
    /// 互斥锁
    MutexType m_mutex;
    /// 最大连接数
    uint32_t m_maxConn;
    /// 数据库连接池，存放已创建的数据库连接对象
    std::map<std::string, std::list<SQLite3*> > m_conns;
    /// 存放数据库创建参数
    std::map<std::string, std::map<std::string, std::string> > m_dbDefines;
};

typedef sylar::Singleton<SQLite3Manager> SQLite3Mgr;


namespace {
template<typename... Args>
int bindX(SQLite3Stmt::ptr stmt, const Args&... args) {
    return SQLite3Binder<1, Args...>::Bind(stmt, args...);
}
}

template<typename... Args>
int SQLite3::execStmt(const char* stmt, Args&&... args) {
    auto st = SQLite3Stmt::Create(shared_from_this(), stmt);
    if(!st) {
        return -1;
    }
    int rt = bindX(st, args...);
    if(rt != SQLITE_OK) {
        return rt;
    }
    return st->execute();
}

template<class... Args>
ISQLData::ptr SQLite3::queryStmt(const char* stmt, const Args&... args) {
    auto st = SQLite3Stmt::Create(shared_from_this(), stmt);
    if(!st) {
        return nullptr;
    }
    int rt = bindX(st, args...);
    if(rt != SQLITE_OK) {
        return nullptr;
    }
    return st->query();
}

namespace {

template<size_t N, typename Head, typename... Tail>
struct SQLite3Binder<N, Head, Tail...> {
    static int Bind(SQLite3Stmt::ptr stmt
                    ,const Head&, const Tail&...) {
        //static_assert(false, "invalid type");
        static_assert(sizeof...(Tail) < 0, "invalid type");
        return SQLITE_OK;
    }
};

#define XX(type, type2) \
template<size_t N, typename... Tail> \
struct SQLite3Binder<N, type, Tail...> { \
    static int Bind(SQLite3Stmt::ptr stmt \
                    , const type2& value \
                    , const Tail&... tail) { \
        int rt = stmt->bind(N, value); \
        if(rt != SQLITE_OK) { \
            return rt; \
        } \
        return SQLite3Binder<N + 1, Tail...>::Bind(stmt, tail...); \
    } \
};

//template<size_t N, typename... Tail>
//struct SQLite3Binder<N, const char(&)[], Tail...> {
//    static int Bind(SQLite3Stmt::ptr stmt
//                    , const char value[]
//                    , const Tail&... tail) {
//        int rt = stmt->bind(N, (const char*)value);
//        if(rt != SQLITE_OK) {
//            return rt;
//        }
//        return SQLite3Binder<N + 1, Tail...>::Bind(stmt, tail...);
//    }
//};

XX(char*, char* const);
XX(const char*, char* const);
XX(std::string, std::string);
XX(int8_t, int32_t);
XX(uint8_t, int32_t);
XX(int16_t, int32_t);
XX(uint16_t, int32_t);
XX(int32_t, int32_t);
XX(uint32_t, int32_t);
XX(int64_t, int64_t);
XX(uint64_t, int64_t);
XX(float, double);
XX(double, double);
#undef XX
}


}


#endif