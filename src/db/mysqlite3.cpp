#include "mysqlite3.h"
#include "log.h"
#include "config.h"
#include "env.h"

namespace sylar{

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static sylar::ConfigVar<std::map<std::string, std::map<std::string, std::string> > >::ptr g_sqlite3_dbs
    = sylar::Config::Lookup("sqlite3.dbs", std::map<std::string, std::map<std::string, std::string> >()
                            , "sqlite3 dbs");


// 构造函数
SQLite3::SQLite3(sqlite3* db)
    :m_db(db) {
}

// 析构函数，关闭数据库
SQLite3::~SQLite3() {
    close();
}

// 创建数据库
SQLite3::ptr SQLite3::Create(sqlite3* db) {
    SQLite3::ptr rt(new SQLite3(db));
    return rt;

    // 若使用make_shared，则构造函数不能在private中
    // 因为make_shared不是本类的函数，相当于类外访问类内private
    // return std::make_shared<SQLite3>(db);
}

// 打开/创建数据库
SQLite3::ptr SQLite3::Create(const std::string& dbname, int flags) {
    sqlite3* db;
    // 以 flags 方式打开数据库，flags选择如下：
    //   SQLITE_OPEN_READONLY: 只读方式打开
    //   SQLITE_OPEN_READWRTEI: 读写方式打开
    //   SQLITE_OPEN_CREATE: 创建数据库
    //   SQLITE_OPEN_READWRTEI | SQLITE_OPEN_CREATE: 以读写方式打开，若数据库不存在则创建它
    if(sqlite3_open_v2(dbname.c_str(), &db, flags, nullptr) == SQLITE_OK) {
        return SQLite3::ptr(new SQLite3(db));
        // return std::make_shared<SQLite3>(db);
    }
    return nullptr;
}

// 编译字符串sql语句，生成sqlite3_stmt对象
IStmt::ptr SQLite3::prepare(const std::string& stmt) {
    return SQLite3Stmt::Create(shared_from_this(), stmt.c_str());
}

// 获取最近一次数据库操作的错误码
int SQLite3::getErrno() {
    return sqlite3_errcode(m_db);
}

// 获取最近一次数据库操作的错误信息
std::string SQLite3::getErrStr() {
    return sqlite3_errmsg(m_db);
}

// 格式化执行SQL语句
int SQLite3::execute(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    int rt = execute(format, ap);
    va_end(ap);
    return rt;
}

int SQLite3::execute(const char* format, va_list ap) {
    // sqlite3_vmprintf函数将格式化的字符串和参数列表转换为SQL语句，并将结果存储format中
    // sqlite3_free函数作为删除器，以确保在指针不再需要时正确释放内存
    std::shared_ptr<char> sql(sqlite3_vmprintf(format, ap), sqlite3_free);
    return sqlite3_exec(m_db, sql.get(), 0, 0, 0);
}

// 格式化执行SQL语句
int SQLite3::execute(const std::string& sql) {
    // 将SQL语句作为字符串传递给SQLite引擎，然后执行这些语句
    return sqlite3_exec(m_db, sql.c_str(), 0, 0, 0);
}

// 获取最近插入行的行ID
int64_t SQLite3::getLastInsertId() {
    // 每个表格都有一个自增长的主键，当插入新行时，SQLite会自动分配一个新的行ID
    // 通过这个函数，可以获取最新插入行的ID
    return sqlite3_last_insert_rowid(m_db);
}

// 格式化获取数据库查询结果
ISQLData::ptr SQLite3::query(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    std::shared_ptr<char> sql(sqlite3_vmprintf(format, ap), sqlite3_free);
    va_end(ap);
    auto stmt = SQLite3Stmt::Create(shared_from_this(), sql.get());
    if(!stmt) {
        return nullptr;
    }
    return stmt->query();
}

// 获取数据库查询结果
ISQLData::ptr SQLite3::query(const std::string& sql) {
    auto stmt = SQLite3Stmt::Create(shared_from_this(), sql.c_str());
    if(!stmt) {
        return nullptr;
    }
    return stmt->query();
}

// 开启事务
ITransaction::ptr SQLite3::openTransaction(bool auto_commit) {
    ITransaction::ptr trans(new SQLite3Transaction(shared_from_this(), auto_commit));
    if(trans->begin()) {
        return trans;
    }
    return nullptr;
}

// 关闭数据库
int SQLite3::close() {
    int rt = SQLITE_OK;
    if(m_db) {
        rt = sqlite3_close(m_db);
        if(rt == SQLITE_OK) {       //关闭没出错
            m_db = nullptr;
        }
    }
    return rt;
}


// 构造函数
SQLite3Data::SQLite3Data(std::shared_ptr<SQLite3Stmt> stmt, int err, const char* errstr) 
    :m_errno(err)
    ,m_first(true)
    ,m_errstr(errstr)
    ,m_stmt(stmt) {
}

/*
    int columnCount = sqlite3_column_count(statement); // 获取整个结果集中的列数

    // 用于在处理查询结果时逐行读取数据
    // sqlite3_step函数用于执行查询并返回下一行数据，如果返回值是SQLITE_ROW，表示查询结果中还有数据行可供读取
    while(sqlite3_step(statement) == SQLITE_ROW) {
        int dataCount = sqlite3_data_count(statement); // 获取当前行的列数
        ...
        // 在循环体中，可以使用一系列的 sqlite3_column_xxx 函数来获取当前行中每一列的数据
        // sqlite3_column_int(statement, idx) 函数用于获取整型数据，
        // sqlite3_column_text(statement, idx) 函数用于获取文本数据等等
    }
*/

// 获取当前查询结果中的列数
int SQLite3Data::getDataCount() {
    return sqlite3_data_count(m_stmt->m_stmt);
}

// 获取整个查询结果集中的列数
int SQLite3Data::getColumnCount() {
    return sqlite3_column_count(m_stmt->m_stmt);
}

// 获取查询结果中指定列的数据长度
int SQLite3Data::getColumnBytes(int idx) {
    return sqlite3_column_bytes(m_stmt->m_stmt, idx);
}

// 获取查询结果中指定列的数据类型
int SQLite3Data::getColumnType(int idx) {
    return sqlite3_column_type(m_stmt->m_stmt, idx);
}

// 获取查询结果中指定列的列名
std::string SQLite3Data::getColumnName(int idx) {
    const char* name = sqlite3_column_name(m_stmt->m_stmt, idx);
    if(name) {
        return name;
    }
    return "";
}

bool SQLite3Data::isNull(int idx) {
    return false;
}

// 获取查询结果中指定列的整数值int
int8_t SQLite3Data::getInt8(int idx) {
    return getInt32(idx);
}

uint8_t SQLite3Data::getUint8(int idx) {
    return getInt32(idx);
}

int16_t SQLite3Data::getInt16(int idx) {
    return getInt32(idx);
}

uint16_t SQLite3Data::getUint16(int idx) {
    return getInt32(idx);
}

int32_t SQLite3Data::getInt32(int idx) {
    /**
     * @brief 用于获取查询结果中指定列的整数值
     * @param[in] idx 是指定列的索引
    */
    return sqlite3_column_int(m_stmt->m_stmt, idx);
}

uint32_t SQLite3Data::getUint32(int idx) {
    return getInt32(idx);
}

int64_t SQLite3Data::getInt64(int idx) {
    return sqlite3_column_int64(m_stmt->m_stmt, idx);
}

uint64_t SQLite3Data::getUint64(int idx) {
    return getInt64(idx);
}

// 获取查询结果中指定列的小数double
float SQLite3Data::getFloat(int idx) {
    return getDouble(idx);
}

double SQLite3Data::getDouble(int idx) {
    /**
     * @brief 用于获取查询结果中指定列的浮点数
     * @param[in] idx 是指定列的索引
    */
    return sqlite3_column_double(m_stmt->m_stmt, idx);
}

// 获取查询结果中指定列的字符串text
std::string SQLite3Data::getString(int idx) {
    /**
     * @brief 用于获取查询结果中指定列的字符串
     * @param[in] idx 是指定列的索引
    */
    const char* v = (const char*)sqlite3_column_text(m_stmt->m_stmt, idx);
    if(v) {
        return std::string(v, getColumnBytes(idx));
    }
    return "";
}

// 获取查询结果中指定列的整数值int/小数double/字符串text/二进制blob
std::string SQLite3Data::getBlob(int idx) {
    /**
     * @brief 用于获取查询结果中指定列的二进制数据
     * @param[in] idx 是指定列的索引
    */
    const char* v = (const char*)sqlite3_column_blob(m_stmt->m_stmt, idx);
    if(v) {
        return std::string(v, getColumnType(idx));
    }
    return "";
}

// 获取字符串格式的时间
time_t SQLite3Data::getTime(int idx) {
    auto str = getString(idx);
    return sylar::StrToTime(str.c_str());
}

// 执行sql语句
bool SQLite3Data::next() {
    int rt = m_stmt->step();
    if(m_first) {
        m_errno = m_stmt->getErrno();
        m_errstr = m_stmt->getErrStr();
        m_first = false;
    }
    return rt == SQLITE_ROW;
}


// 构造函数
SQLite3Stmt::SQLite3Stmt(SQLite3::ptr db) 
    :m_db(db) 
    ,m_stmt(nullptr) {
}

// 析构函数，释放sqlite3_stmt对象
SQLite3Stmt::~SQLite3Stmt() {
    finish();
}

// 创建SQLite3Stmt对象，并生成可执行的sql语句实例
SQLite3Stmt::ptr SQLite3Stmt::Create(SQLite3::ptr db, const char* stmt) {
    SQLite3Stmt::ptr st(new SQLite3Stmt(db));
    if(st->prepare(stmt) != SQLITE_OK) {
        return nullptr;
    }
    return st;
}

// sql语句的准备，生成可执行的sql语句实例对象(sqlite3_stmt对象)
int SQLite3Stmt::prepare(const char* stmt) {
    auto rt = finish();
    if(rt != SQLITE_OK) {
        return rt;
    }
    // 将SQL语句编译为一个sqlite3_stmt对象，并进行语法和语义的检查
    // 编译后的语句将存储在m_stmt变量中，以便稍后执行该语句
    return sqlite3_prepare_v2(m_db->getDB(), stmt, strlen(stmt), &m_stmt, nullptr);
}

// 释放由sqlite3_prepare_v2函数编译的SQL语句对象，从内存中删除以避免内存泄漏
int SQLite3Stmt::finish() {
    int rt = SQLITE_OK;
    if(m_stmt) {
        // 函数用于释放由sqlite3_prepare_v2函数编译的SQL语句对象
        // 它的作用是在使用完语句对象后，将其从内存中删除，以避免内存泄漏
        rt = sqlite3_finalize(m_stmt);
        if(rt == SQLITE_OK) {
            m_stmt = nullptr;
        }
    }
    return rt;
}

// sqlite3_bind_int 用于将整型值绑定到一个预编译的SQL语句中的参数上
// 使用 sqlite3_prepare_v2 函数编译SQL语句时，可以使用 ? 占位符来表示参数，
// 通过 sqlite3_bind_xxx 系列函数，可以将具体的值绑定到这些占位符上。
/*
    sqlite3_stmt* stmt;
    const char* sql = "SELECT * FROM my_table WHERE id = ?";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, 42);  // 将42绑定到SQL语句的第一个参数
*/
int SQLite3Stmt::bind(int idx, int32_t value) {
    // 将一个整型值绑定到预编译的SQL语句中的参数上
    // m_stmt是一个预编译的SQL语句对象，idx是占位符的索引，value是要绑定的整型值
    return sqlite3_bind_int(m_stmt, idx, value);
}

int SQLite3Stmt::bind(int idx, uint32_t value) {
    return sqlite3_bind_int(m_stmt, idx, value);
}

int SQLite3Stmt::bind(int idx, double value) {
    return sqlite3_bind_double(m_stmt, idx, value);
}

int SQLite3Stmt::bind(int idx, int64_t value) {
    return sqlite3_bind_int64(m_stmt, idx, value);
}

int SQLite3Stmt::bind(int idx, uint64_t value) {
    return sqlite3_bind_int64(m_stmt, idx, value);
}

int SQLite3Stmt::bind(int idx, const char* value, Type type) {
    // 将一个文本值绑定到预编译的SQL语句中的参数上
    // m_stmt是一个预编译的SQL语句对象，idx是占位符的索引，value是要绑定的整型值
    // SQLITE_TRANSIENT 表示在绑定期间进行内存管理，即在绑定期间将参数值复制到内部缓冲区，并在适当的时候释放内存
    // SQLITE_STATIC 表示参数值已经被分配到静态内存中，不需要在绑定期间进行内存管理
    return sqlite3_bind_text(m_stmt, idx, value, strlen(value)
                            ,type == COPY ? SQLITE_TRANSIENT : SQLITE_STATIC);
}

int SQLite3Stmt::bind(int idx, const void* value, int len, Type type) {
    return sqlite3_bind_blob(m_stmt, idx, value, len
                            ,type == COPY ? SQLITE_TRANSIENT : SQLITE_STATIC);
}

int SQLite3Stmt::bind(int idx, const std::string& value, Type type) {
    return sqlite3_bind_text(m_stmt, idx, value.c_str(), value.size()
                            ,type == COPY ? SQLITE_TRANSIENT : SQLITE_STATIC);
}

// for null type
int SQLite3Stmt::bind(int idx) {
    // 当字符串为NULL时，使用 sqlite3_bind_null 绑定
    return sqlite3_bind_null(m_stmt, idx);
}

int SQLite3Stmt::bindInt8(int idx, const int8_t& value) {
    return bind(idx, (int32_t)value);
}

int SQLite3Stmt::bindUint8(int idx, const uint8_t& value) {
    return bind(idx, (int32_t)value);
}

int SQLite3Stmt::bindInt16(int idx, const int16_t& value) {
    return bind(idx, (int32_t)value);
}

int SQLite3Stmt::bindUint16(int idx, const uint16_t& value) {
    return bind(idx, (int32_t)value);
}

int SQLite3Stmt::bindInt32(int idx, const int32_t& value) {
    return bind(idx, (int32_t)value);
}

int SQLite3Stmt::bindUint32(int idx, const uint32_t& value) {
    return bind(idx, (int32_t)value);
}

int SQLite3Stmt::bindInt64(int idx, const int64_t& value) {
    return bind(idx, (int64_t)value);
}

int SQLite3Stmt::bindUint64(int idx, const uint64_t& value) {
    return bind(idx, (int64_t)value);
}

int SQLite3Stmt::bindFloat(int idx, const float& value) {
    return bind(idx, (double)value);
}

int SQLite3Stmt::bindDouble(int idx, const double& value) {
    return bind(idx, (double)value);
}

int SQLite3Stmt::bindString(int idx, const char* value) {
    return bind(idx, value);
}

int SQLite3Stmt::bindString(int idx, const std::string& value) {
    return bind(idx, value);
}

// 绑定二进制变量
int SQLite3Stmt::bindBlob(int idx, const void* value, int64_t size) {
    return bind(idx, value, size);
}

int SQLite3Stmt::bindBlob(int idx, const std::string& value) {
    return bind(idx, (void*)&value[0], value.size());
}

// 绑定字符串格式的时间
int SQLite3Stmt::bindTime(int idx, const time_t& value) {
    return bind(idx, sylar::TimeToStr(value));
}

int SQLite3Stmt::bindNull(int idx) {
    return bind(idx);
}

/*
    const char* sql = "SELECT * FROM users WHERE id = :id AND name = :name;";
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    int id = 123;
    const char* name = "wwt";
    sqlite3_bind_int(stmt, sqlite3_bind_parameter_index(stmt, ":id"), id);
    sqlite3_bind_text(stmt, sqlite3_bind_parameter_index(stmt, ":name"), name, -1, SQLITE_STATIC);
*/
// 根据参数索引idx绑定value值到预编译的SQL语句中的参数上
int SQLite3Stmt::bind(const char* name, int32_t value) {
    // 用于获取给定名称name的绑定参数在SQL语句中的索引
    auto idx = sqlite3_bind_parameter_index(m_stmt, name);
    return bind(idx, value);
}

int SQLite3Stmt::bind(const char* name, uint32_t value) {
    auto idx = sqlite3_bind_parameter_index(m_stmt, name);
    return bind(idx, value);
}

int SQLite3Stmt::bind(const char* name, double value) {
    auto idx = sqlite3_bind_parameter_index(m_stmt, name);
    return bind(idx, value);
}

int SQLite3Stmt::bind(const char* name, int64_t value) {
    auto idx = sqlite3_bind_parameter_index(m_stmt, name);
    return bind(idx, value);
}

int SQLite3Stmt::bind(const char* name, uint64_t value) {
    auto idx = sqlite3_bind_parameter_index(m_stmt, name);
    return bind(idx, value);
}

int SQLite3Stmt::bind(const char* name, const char* value, Type type) {
    auto idx = sqlite3_bind_parameter_index(m_stmt, name);
    return bind(idx, value, type);
}

int SQLite3Stmt::bind(const char* name, const void* value, int len, Type type) {
    auto idx = sqlite3_bind_parameter_index(m_stmt, name);
    return bind(idx, value, len, type);
}

int SQLite3Stmt::bind(const char* name, const std::string& value, Type type) {
    auto idx = sqlite3_bind_parameter_index(m_stmt, name);
    return bind(idx, value, type);
}

// for null type
int SQLite3Stmt::bind(const char* name) {
    auto idx = sqlite3_bind_parameter_index(m_stmt, name);
    return bind(idx);
}

// 执行编译好的SQL语句对象m_stmt(执行sql语句)
int SQLite3Stmt::step() {
    /**
     * @return SQLITE_BUSY: 没获取到锁，语句没有执行
     *         SQLITE_ERROR: 出错
     *         SQLITE_MISUSE: 使用方法不当
     *         SQLITE_DONE: SQL语句执行完成
     *         SQLITE_ROW: 当SQL语句为select时，执行后结果是一个表，它是一行一行返回的，
     *           调用一次就会返回一行，SQLITE_ROW就表示有返回结果，一直到返回SQLITE_DONE
    */
    return sqlite3_step(m_stmt);
}

// 复位SQL语句对象m_stmt，以便下一轮的参数的绑定
int SQLite3Stmt::reset() {
    return sqlite3_reset(m_stmt);
}

// 获取ISQLData对象，用于查询
ISQLData::ptr SQLite3Stmt::query() {
    return SQLite3Data::ptr(new SQLite3Data(shared_from_this(), 0, ""));
}

// 执行sql语句
int SQLite3Stmt::execute() {
    int rt = step();
    if(rt == SQLITE_DONE) {
        rt = SQLITE_OK;
    }
    return rt;
}

// 用于获取最近插入行的行ID
int64_t SQLite3Stmt::getLastInsertId() {
    return m_db->getLastInsertId();
}

// 用于获取最近一次数据库操作的错误码
int SQLite3Stmt::getErrno() {
    return m_db->getErrno();
}

// 用于获取最近一次数据库操作的错误信息
std::string SQLite3Stmt::getErrStr() {
    return m_db->getErrStr();
}

// 构造函数
SQLite3Transaction::SQLite3Transaction(SQLite3::ptr db, bool auto_commit, Type type)
    :m_db(db)
    ,m_type(type)
    ,m_status(0)
    ,m_autoCommit(auto_commit) {
}

// 析构函数
SQLite3Transaction::~SQLite3Transaction() {
    // 事务开启但未提交、回滚
    if(m_status == 1) {
        // 设置了自动提交
        if(m_autoCommit) {
            commit();
        } else {
            rollback();
        }

        // 未提交、回滚成功
        if(m_status == 1) {
            SYLAR_LOG_ERROR(g_logger) << m_db << " auto_commit=" << m_autoCommit << " fail";
        }
    }
}

// 开启事务
bool SQLite3Transaction::begin() {
    if(m_status == 0) {
        const char* sql = "BEGIN";
        if(m_type == IMMEDIATE) {
            sql = "BEGIN IMMEDIATE";
        } else if(m_type == EXCLUSIVE) {
            sql = "BEGIN EXCLUSIVE";
        }
        int rt = m_db->execute(sql);
        if(rt == SQLITE_OK) {
            m_status = 1;
        }
        return rt == SQLITE_OK;
    }
    return false;
}

// 提交事务
bool SQLite3Transaction::commit() {
    if(m_status == 1) {
        int rt = m_db->execute("COMMIT");
        if(rt == SQLITE_OK) {
            m_status = 2;
        }
        return rt == SQLITE_OK;
    }
    return false;
}

// 回滚事务
bool SQLite3Transaction::rollback() {
    if(m_status == 1) {
        int rt = m_db->execute("ROLLBACK");
        if(rt == SQLITE_OK) {
            m_status = 3;
        }
        return rt == SQLITE_OK;
    }
    return false;
}

// 格式化执行QL语句
int SQLite3Transaction::execute(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    int rt = m_db->execute(format, ap);
    va_end(ap);
    return rt;
}

// 执行SQL语句
int SQLite3Transaction::execute(const std::string& sql) {
    return m_db->execute(sql);
}

// 用于获取最近插入行的行ID
int64_t SQLite3Transaction::getLastInsertId() {
    return m_db->getLastInsertId();
}


// 构造函数
SQLite3Manager::SQLite3Manager()
    :m_maxConn(10) {
}

// 析构函数
SQLite3Manager::~SQLite3Manager() {
    for(auto& i : m_conns) {
        for(auto& n : i.second) {
            delete n;
        }
    }
}

// 获取数据库连接对象
SQLite3::ptr SQLite3Manager::get(const std::string& name) {
    // 有对应名称name的数据库连接，直接拿出返回
    MutexType::Lock lock(m_mutex);
    auto it = m_conns.find(name);
    if(it != m_conns.end()) {
        if(!it->second.empty()) {
            SQLite3* rt = it->second.front();
            it->second.pop_front();
            lock.unlock();
            return SQLite3::ptr(rt, std::bind(&SQLite3Manager::freeSQLite3,
                                this, name, std::placeholders::_1));
        }
    }

    // 没有对应名称name的数据库连接，需要根据参数创建
    // 如果有配置参数就拿配置，没有就拿m_dbDefines
    // sqlite3:
    //     dbs:
    //         name:
    //             path: ...
    //             sql: ...
    //             ...
    auto config = g_sqlite3_dbs->getValue();
    auto sit = config.find(name);
    std::map<std::string, std::string> args;
    if(sit != config.end()) {
        args = sit->second;
    } else {
        sit = m_dbDefines.find(name);
        if(sit != m_dbDefines.end()) {
            args = sit->second;
        } else {
            return nullptr;
        }
    }
    lock.unlock();

    // 找创建数据库的路径
    std::string path = GetParamValue<std::map<std::string, std::string>
                                    , std::string, std::string>(args, "path");
    if(path.empty()) {
        SYLAR_LOG_ERROR(g_logger) << "open db name=" << name << " path is null";
        return nullptr;
    }
    if(path.find(":") == std::string::npos) {
        path = sylar::EnvMgr::GetInstance()->getAbsoluteWorkPath(path);
    }

    // 创建数据库
    sqlite3* db;
    // 表示以读写方式打开数据库，并在数据库不存在时创建它
    if(sqlite3_open_v2(path.c_str(), &db, SQLite3::CREATE | SQLite3::READWRITE, nullptr)) {
        SYLAR_LOG_ERROR(g_logger) << "open db name=" << name << " path=" << path << " fail";
        return nullptr;
    }

    // 创建数据库连接对象封装，并执行sql语句
    SQLite3* rt = new SQLite3(db);
    std::string sql = GetParamValue<std::map<std::string, std::string>
                                    , std::string, std::string>(args, "sql");
    if(!sql.empty()) {
        if(rt->execute(sql)) {
            SYLAR_LOG_ERROR(g_logger) << "execute sql=" << sql
                << " errno=" << rt->getErrno() << " errstr=" << rt->getErrStr();
            delete rt;
            return nullptr;
        }
    }
    rt->m_lastUsedTime = time(0);
    return SQLite3::ptr(rt, std::bind(&SQLite3Manager::freeSQLite3,
                                this, name, std::placeholders::_1));
}

// 设置数据库创建参数
void SQLite3Manager::registerSQLite3(const std::string& name, const std::map<std::string, std::string>& params) {
    MutexType::Lock lock(m_mutex);
    m_dbDefines[name] = params;
}

// 更新连接池m_conns
void SQLite3Manager::checkConnection(int sec) {
    time_t now = time(0);
    std::vector<SQLite3*> conns;
    MutexType::Lock lock(m_mutex);
    for(auto& i : m_conns) {
        for(auto it = i.second.begin();
                it != i.second.end();) {
            if((int)(now - (*it)->m_lastUsedTime) >= sec) {
                auto tmp = *it;
                i.second.erase(it++);
                conns.push_back(tmp);
            } else {
                ++it;
            }
        }
    }
    lock.unlock();
    for(auto& i : conns) {
        delete i;
    }
}

// 让指定数据库去格式化执行SQL语句
int SQLite3Manager::execute(const std::string& name, const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    int rt = execute(name, format, ap);
    va_end(ap);
    return rt;
}

int SQLite3Manager::execute(const std::string& name, const char* format, va_list ap) {
    auto conn = get(name);
    if(!conn) {
        SYLAR_LOG_ERROR(g_logger) << "SQLite3Manager::execute, get(" << name
            << ") fail, format=" << format;
        return -1;
    }
    return conn->execute(format, ap);
}

// 让指定数据库去执行SQL语句
int SQLite3Manager::execute(const std::string& name, const std::string& sql) {
    auto conn = get(name);
    if(!conn) {
        SYLAR_LOG_ERROR(g_logger) << "SQLite3Manager::execute, get(" << name
            << ") fail, sql=" << sql;
        return -1;
    }
    return conn->execute(sql);
}

// 让指定数据库去格式化获取数据库查询结果
ISQLData::ptr SQLite3Manager::query(const std::string& name, const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    auto res = query(name, format, ap);
    va_end(ap);
    return res;
}

ISQLData::ptr SQLite3Manager::query(const std::string& name, const char* format, va_list ap) {
    auto conn = get(name);
    if(!conn) {
        SYLAR_LOG_ERROR(g_logger) << "SQLite3Manager::query, get(" << name
            << ") fail, format=" << format;
        return nullptr;
    }
    return conn->query(format, ap);
}

// 让指定数据库去获取数据库查询结果
ISQLData::ptr SQLite3Manager::query(const std::string& name, const std::string& sql) {
    auto conn = get(name);
    if(!conn) {
        SYLAR_LOG_ERROR(g_logger) << "SQLite3Manager::query, get(" << name
            << ") fail, sql=" << sql;
        return nullptr;
    }
    return conn->query(sql);

}

SQLite3Transaction::ptr SQLite3Manager::openTransaction(const std::string& name, bool auto_commit) {
    auto conn = get(name);
    if(!conn) {
        SYLAR_LOG_ERROR(g_logger) << "SQLite3Manager::openTransaction, get(" << name
            << ") fail";
        return nullptr;
    }
    SQLite3Transaction::ptr trans(new SQLite3Transaction(conn, auto_commit));
    return trans;
}

// 释放数据库连接对象
void SQLite3Manager::freeSQLite3(const std::string& name, SQLite3* m) {
    if(m->m_db) {
        MutexType::Lock lock(m_mutex);
        if(m_conns[name].size() < m_maxConn) {
            m_conns[name].push_back(m);
            return;
        }
    }
    delete m;
}


} // namespace sylar
