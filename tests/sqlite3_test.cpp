#include "mysqlite3.h"
#include "log.h"
#include "util.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

// 事务性能测试
void test_batch(sylar::SQLite3::ptr db) {
    auto ts = sylar::GetCurrentMS();
    int n = 1000000;
    sylar::SQLite3Transaction trans(db);
    //db->execute("PRAGMA synchronous = OFF;");

    // 开启事务
    trans.begin();

    // 执行sql语句
    sylar::SQLite3Stmt::ptr stmt = sylar::SQLite3Stmt::Create(db,
                    "insert into user(name, age) values(?, ?)");
    for(int i = 0; i < n; ++i) {
        stmt->reset();
        stmt->bind(1, "batch_" + std::to_string(i));
        stmt->bind(2, i);
        stmt->step();
    }

    // 提交事务
    trans.commit();

    auto ts2 = sylar::GetCurrentMS();
    SYLAR_LOG_INFO(g_logger) << "used: " << (ts2 - ts) / 1000.0 << "s batch insert n=" << n;
}

int main(int argc, char** argv) {
    // 创建数据库
    const std::string dbname = "test.db";   // 未指定具体路径，则在编译路径下build/
    auto db = sylar::SQLite3::Create(dbname, sylar::SQLite3::READWRITE);
    if(!db) {
        SYLAR_LOG_INFO(g_logger) << "dbname=" << dbname << " not exists";
        db = sylar::SQLite3::Create(dbname
                    , sylar::SQLite3::READWRITE | sylar::SQLite3::CREATE);
        if(!db) {
            SYLAR_LOG_INFO(g_logger) << "dbname=" << dbname << " create error";
            return 0;
        }

    // 创建表格
#define XX(...) #__VA_ARGS__
        int rt = db->execute(
XX(create table user (
        id integer primary key autoincrement,
        name varchar(50) not null default "",
        age int not null default 0,
        create_time datetime
        )));
#undef XX

        if(rt != SQLITE_OK) {
            SYLAR_LOG_ERROR(g_logger) << "create table error "
                << db->getErrno() << " - " << db->getErrStr();
            return 0;
        }
    }

    // 用 sqlite3_exec 插入数据
    // 将prepare、bind、step封装在一起
    // 相当于每次循环都会执行这三个步骤
    for(int i = 0; i < 10; ++i) {
        if(db->execute("insert into user(name, age) values(\"exec_%d\", %d)", i, i)
                    != SQLITE_OK) {
            SYLAR_LOG_ERROR(g_logger) << "insert into error " << i << " "
                            << db->getErrno() << " - " << db->getErrStr();
        }
    }

    // 用 sqlite3_stmt 插入数据
    // 先prepare，再bind，最后step
    // prepare完之后，每次循环只需要bind不同的参数去step，效率高一些
    sylar::SQLite3Stmt::ptr stmt = sylar::SQLite3Stmt::Create(db,
                "insert into user(name, age, create_time) values(?, ?, ?)");
    if(!stmt) {
        SYLAR_LOG_ERROR(g_logger) << "create statement error "
            << db->getErrno() << " - " << db->getErrStr();
        return 0;
    }

    int64_t now = time(0);
    for(int i = 0; i < 10; ++i) {
        stmt->bind(1, "stmt_" + std::to_string(i));
        stmt->bind(2, i);
        stmt->bind(3, now + rand() % 100);
        //stmt->bind(3, sylar::Time2Str(now + rand() % 100));
        //stmt->bind(3, "stmt_" + std::to_string(i + 1));
        //stmt->bind(4, i + 1);

        if(stmt->execute() != SQLITE_OK) {
            SYLAR_LOG_ERROR(g_logger) << "execute statment error " << i << " "
                << db->getErrno() << " - " << db->getErrStr();
        }
        stmt->reset();
    }

    // 查询数据
    sylar::SQLite3Stmt::ptr query = sylar::SQLite3Stmt::Create(db, "select * from user");
    if(!query) {
        SYLAR_LOG_ERROR(g_logger) << "create statement error "
                << db->getErrno() << " - " << db->getErrStr();
        return 0;
    }
    auto ds = query->query();   // 创建ISQLData::ptr
    if(!ds) {
        SYLAR_LOG_ERROR(g_logger) << "query error "
                << db->getErrno() << " - " << db->getErrStr();
        return 0;
    }

    // 遍历查询表的每一行
    while(ds->next()) {
        //SYLAR_LOG_INFO(g_logger) << "query ";
    };

    //const char v[] = "hello ' world";
    const std::string v = "hello ' world";
    // 格式化执行SQL语句，封装了prepare、bind、step三个过程
    db->execStmt("insert into user(name) values (?)", v);

    // 格式化获取数据库查询结果，封装了prepare、bind、query三个过程
    auto dd = (db->queryStmt("select * from user"));

    // 遍历查询表的每一行
    while(dd->next()) {
        SYLAR_LOG_INFO(g_logger) << "ds.data_count=" << dd->getDataCount()
                            << " ds.column_count=" << dd->getColumnCount()
                            << " id=" << dd->getInt32(0) 
                            << " name=" << dd->getString(1)
                            << " age=" << dd->getString(2) 
                            << " time=" << dd->getString(3);
    }

    // 事务性能测试
    test_batch(db);
    return 0;
}
