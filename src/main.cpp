
#include "DispatchMsgService.h"
#include "interface.h"
#include "Logger.h"
#include "sqlconnection.h"
#include "BusProcessor.h"
#include "iniconfig.h"
#include "configdef.h"

#include <functional>

 extern "C"{
 #include <lua.h>
 #include <lualib.h>
 #include <lauxlib.h>
 //#pragma comment(lib, "lua.lib")
 }

 lua_State* initLuaEnv()
 {
 	lua_State* luaEnv = luaL_newstate();
 	luaopen_base(luaEnv);
 	luaL_openlibs(luaEnv);
  
 	return luaEnv;
 }

 bool loadLuaFile(lua_State* luaEnv, const string& fileName)
 {
 	int result = luaL_loadfile(luaEnv, fileName.c_str());
 	if (result)
 	{
 		return false;
 	}
 	
 	result = lua_pcall(luaEnv, 0, 0, 0);
 	return result == 0;
 }

 lua_CFunction getGlobalProc(lua_State* luaEnv, const string& procName)
 {
 	lua_getglobal(luaEnv, procName.c_str());
 	if (!lua_iscfunction(luaEnv, 1))
 	{
 		return 0;
 	}
  
 	return lua_tocfunction(luaEnv, 1);
 }

 #if 1
 int main(int argc, char* argv[])
 {
 	
 	if (argc != 2)
 	{
 		printf("please input brks <log file config>!\n");
 		return -1;
 	}
 	
 	if(!Logger::instance()->init(std::string(argv[1])))
 	{
 		printf("init log module failed.\n");
 		return -1;
 	}
 	else
 	{
 		printf("init log module success!\n");
 	}
  

 	lua_State* luaEnv = initLuaEnv();
 	string host, dbUserName, dbPassWd, myDb;
 	int port, dbPort;
 	
 	if (!luaEnv)
 	{
 		return -1;
 	}

 	if (!loadLuaFile(luaEnv, "conf.lua"))
 	{
 		cout<<"Load Lua File FAILED!"<<endl;
 		return -1;
 	}

 	lua_getglobal(luaEnv, "host");
     if (lua_isstring(luaEnv, -1)) {
         host = (char*)lua_tostring(luaEnv, -1);
         std::cout << "host = " << host << std::endl;
     }
     lua_pop(luaEnv, 1);

     lua_getglobal(luaEnv, "port");
     if (lua_isnumber(luaEnv, -1)) {
         port = (int)lua_tonumber(luaEnv, -1);
         std::cout << "port = " << port << std::endl;
     }
     lua_pop(luaEnv, 1);

     lua_getglobal(luaEnv, "dbUserName");
         if (lua_isstring(luaEnv, -1)) {
         dbUserName = (char*)lua_tostring(luaEnv, -1);
         std::cout << "dbUserName = " << dbUserName << std::endl;
     }
     lua_pop(luaEnv, 1);

     lua_getglobal(luaEnv, "dbPassWd");
         if (lua_isstring(luaEnv, -1)) {
         dbPassWd = (char*)lua_tostring(luaEnv, -1);
         std::cout << "dbPassWd = " << dbPassWd << std::endl;
     }
     lua_pop(luaEnv, 1);

     lua_getglobal(luaEnv, "db");
     if (lua_isstring(luaEnv, -1)) {
         myDb = (char*)lua_tostring(luaEnv, -1);
         std::cout << "db = " << myDb << std::endl;
     }
     lua_pop(luaEnv, 1);

     lua_getglobal(luaEnv, "dbPort");
     if (lua_isnumber(luaEnv, -1)) {
         dbPort = (int)lua_tonumber(luaEnv, -1);
         std::cout << "dbPort = " << dbPort << std::endl;
     }
     lua_pop(luaEnv, 1);
  
 	lua_close(luaEnv);

 	std::shared_ptr<DispatchMsgService> dms(new DispatchMsgService);
 	dms->open();

 	std::shared_ptr<MysqlConnection> mysqlconn(new MysqlConnection);
 	mysqlconn->Init(host.c_str(), port, dbUserName.c_str(), dbPassWd.c_str(), myDb.c_str());
 	
 	BusinessProcessor processor(dms, mysqlconn);
 	processor.init();

 	std::function<iEvent *(const iEvent *)> fun = std::bind(&DispatchMsgService::process, dms.get(), std::placeholders::_1); 

 	Interface intf(fun);
 	intf.start(dbPort);

 	LOG_INFO("brks start successful!");

 	for(;;);

     return 0;
 }

#else

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        printf("please input brks <log file config> <config file path>!\n");
        return -1;
    }

    if(!Logger::instance()->init(std::string(argv[1])))
    {
        printf("init log module failed.\n");
        return -2;
    }
    else
    {
        printf("init log module success!");
    }

    Iniconfig config;

    if (!config.loadfile(std::string(argv[2])))
    {
        LOG_ERROR("load %s failed.", argv[2]);
        return -3;
    }
    st_env_config conf_args = config.getconfig();

    std::shared_ptr<DispatchMsgService> dms(new DispatchMsgService);
    dms->open();

    std::shared_ptr<MysqlConnection> mysqlconn(new MysqlConnection);
    //mysqlconn->Init("127.0.0.1", 3306, "root", "123456", "dongnaobike");
    mysqlconn->Init(conf_args.db_ip.c_str(), conf_args.db_port, \
        conf_args.db_user.c_str(), conf_args.db_pwd.c_str(), conf_args.db_name.c_str());

    BusinessProcessor processor(dms, mysqlconn);
    processor.init();

    std::function< iEvent* (const iEvent*)> fun = std::bind(&DispatchMsgService::process, dms.get(), std::placeholders::_1);

    Interface intf(fun);
    intf.start(conf_args.svr_port);

    LOG_INFO("brks start successful!");

    for(;;);

    return 0;
}
#endif
