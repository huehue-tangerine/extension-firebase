#include "luautils.h"
#include <dmsdk/sdk.h>

#define LIB_NAME "Firebase"
#define MODULE_NAME "firebase"
#define DLIB_LOG_DOMAIN LIB_NAME

#if !defined(DM_PLATFORM_HTML5)

#include "firebase/app.h"
#include "firebase/analytics.h"
#include "firebase/analytics/event_names.h"
#include "firebase/analytics/parameter_names.h"
#include "firebase/future.h"

#if defined(DM_PLATFORM_ANDROID)
static JNIEnv* GetJNIEnv()
{
	JNIEnv* env = 0;
	dmGraphics::GetNativeAndroidJavaVM()->AttachCurrentThread(&env, NULL);
	return env;
}
#endif


static firebase::App* firebase_app_;

using namespace firebase;
using namespace firebase::analytics;

static int Firebase_Init(lua_State* L) {
	dmLogInfo("Firebase_Init");
	int top = lua_gettop(L);

	if (lua_type(L, 1) == LUA_TSTRING) {
		const char* config = luaL_checkstring(L, 1);
		AppOptions* options = AppOptions::LoadFromJsonConfig(config, 0);
		#if defined(DM_PLATFORM_ANDROID)
		dmLogInfo("Creating app");
		firebase_app_ = App::Create(*options, GetJNIEnv(), dmGraphics::GetNativeAndroidActivity());
		#else
		firebase_app_ = App::Create(*options);
		#endif
	}
	else {
		#if defined(DM_PLATFORM_ANDROID)
		dmLogInfo("Creating app");
		firebase_app_ = App::Create(GetJNIEnv(), dmGraphics::GetNativeAndroidActivity());
		#else
		firebase_app_ = App::Create();
		#endif
	}

	if(!firebase_app_) {
		dmLogError("firebase::App::Create failed");
		return 0;
	}
	analytics::Initialize(*firebase_app_);

	dmLogInfo("Firebase_Init done");
	assert(top == lua_gettop(L));
	return 0;
}




lua_Listener getInstanceIdListener;

static int Firebase_Analytics_InstanceId(lua_State* L) {
	int top = lua_gettop(L);

	luaL_checklistener(L, 1, getInstanceIdListener);

	void* user_data = 0;
	Future<std::string> future = analytics::GetAnalyticsInstanceId();
	future.OnCompletion([](const Future< std::string >& completed_future, void* user_data) {
			lua_State* L = (lua_State*)user_data;
			if (completed_future.error() == 0) {
				lua_pushlistener(L, getInstanceIdListener);
				lua_pushstring(L, completed_future.result()->c_str());
				int ret = lua_pcall(L, 2, 0, 0);
				if (ret != 0) {
					lua_pop(L, 1);
				}
			}
			else {
				dmLogError("%d: %s", completed_future.error(), completed_future.error_message());
				lua_pushlistener(L, getInstanceIdListener);
				lua_pushnil(L);
				lua_pushstring(L, completed_future.error_message());
				int ret = lua_pcall(L, 3, 0, 0);
				if (ret != 0) {
					lua_pop(L, 2);
				}
			}
		}, L);

	assert(top == lua_gettop(L));
	return 0;
}

static int Firebase_Analytics_Log(lua_State* L) {
	int top = lua_gettop(L);

	const char* name = luaL_checkstring(L, 1);
	LogEvent(name);

	assert(top == lua_gettop(L));
	return 0;
}

static int Firebase_Analytics_LogString(lua_State* L) {
	int top = lua_gettop(L);

	const char* name = luaL_checkstring(L, 1);
	const char* parameter_name = luaL_checkstring(L, 2);
	const char* parameter_value = luaL_checkstring(L, 3);
	LogEvent(name, parameter_name, parameter_value);

	assert(top == lua_gettop(L));
	return 0;
}

static int Firebase_Analytics_LogInt(lua_State* L) {
	int top = lua_gettop(L);

	const char* name = luaL_checkstring(L, 1);
	const char* parameter_name = luaL_checkstring(L, 2);
	const int parameter_value = luaL_checkint(L, 3);
	LogEvent(name, parameter_name, parameter_value);

	assert(top == lua_gettop(L));
	return 0;
}

static int Firebase_Analytics_LogNumber(lua_State* L) {
	int top = lua_gettop(L);

	const char* name = luaL_checkstring(L, 1);
	const char* parameter_name = luaL_checkstring(L, 2);
	const lua_Number parameter_value = luaL_checknumber(L, 3);
	LogEvent(name, parameter_name, parameter_value);

	assert(top == lua_gettop(L));
	return 0;
}

static int Firebase_Analytics_Reset(lua_State* L) {
	int top = lua_gettop(L);
	ResetAnalyticsData();
	assert(top == lua_gettop(L));
	return 0;
}

static int Firebase_Analytics_SetEnabled(lua_State* L) {
	int top = lua_gettop(L);
	int enabled = lua_toboolean(L, 1);
	SetAnalyticsCollectionEnabled((bool)enabled);
	assert(top == lua_gettop(L));
	return 0;
}

static int Firebase_Analytics_SetScreen(lua_State* L) {
	int top = lua_gettop(L);

	const char* screen_name = luaL_checkstring(L, 1);
	const char* screen_class = luaL_checkstring(L, 2);
	SetCurrentScreen(screen_name, screen_class);

	assert(top == lua_gettop(L));
	return 0;
}

static int Firebase_Analytics_SetUserProperty(lua_State* L) {
	int top = lua_gettop(L);

	const char* name = luaL_checkstring(L, 1);
	const char* property = luaL_checkstring(L, 2);
	SetUserProperty(name, property);

	assert(top == lua_gettop(L));
	return 0;
}

static int Firebase_Analytics_SetUserId(lua_State* L) {
	int top = lua_gettop(L);

	const char* user_id = luaL_checkstring(L, 1);
	SetUserId(user_id);

	assert(top == lua_gettop(L));
	return 0;
}

static int Firebase_Analytics_SetMinimumSessionDuration(lua_State* L) {
	int top = lua_gettop(L);
	int duration = luaL_checkint(L, 1);
	SetMinimumSessionDuration(duration);
	assert(top == lua_gettop(L));
	return 0;
}


static const luaL_reg Module_methods[] = {
	{"init", Firebase_Init},
	{0, 0}
};

static void LuaInit(lua_State* L) {
	int top = lua_gettop(L);
	luaL_register(L, MODULE_NAME, Module_methods);

	lua_pushstring(L, "analytics");
	lua_newtable(L);
	lua_pushtablestringfunction(L, "get_id", Firebase_Analytics_InstanceId);
	lua_pushtablestringfunction(L, "log", Firebase_Analytics_Log);
	lua_pushtablestringfunction(L, "log_string", Firebase_Analytics_LogString);
	lua_pushtablestringfunction(L, "log_int", Firebase_Analytics_LogInt);
	lua_pushtablestringfunction(L, "log_number", Firebase_Analytics_LogNumber);
	lua_pushtablestringfunction(L, "reset", Firebase_Analytics_Reset);
	lua_pushtablestringfunction(L, "set_enabled", Firebase_Analytics_SetEnabled);
	lua_pushtablestringfunction(L, "set_screen", Firebase_Analytics_SetScreen);
	lua_pushtablestringfunction(L, "set_user_property", Firebase_Analytics_SetUserProperty);
	lua_pushtablestringfunction(L, "set_user_id", Firebase_Analytics_SetUserId);
	lua_pushtablestringfunction(L, "set_minimum_session_duration", Firebase_Analytics_SetMinimumSessionDuration);
	lua_settable(L, -3);

	lua_pop(L, 1);

	assert(top == lua_gettop(L));
}

#endif

dmExtension::Result AppInitializeFirebaseExtension(dmExtension::AppParams* params) {
	return dmExtension::RESULT_OK;
}

dmExtension::Result InitializeFirebaseExtension(dmExtension::Params* params) {
	#if defined(DM_PLATFORM_HTML5)
		printf("Extension %s is not supported\n", MODULE_NAME);
	#else
		LuaInit(params->m_L);
	#endif
	return dmExtension::RESULT_OK;
}

dmExtension::Result AppFinalizeFirebaseExtension(dmExtension::AppParams* params) {
	return dmExtension::RESULT_OK;
}

dmExtension::Result FinalizeFirebaseExtension(dmExtension::Params* params) {
	return dmExtension::RESULT_OK;
}

dmExtension::Result UpdateFirebaseExtension(dmExtension::Params* params) {
	return dmExtension::RESULT_OK;
}

DM_DECLARE_EXTENSION(Firebase, LIB_NAME, AppInitializeFirebaseExtension, AppFinalizeFirebaseExtension, InitializeFirebaseExtension, UpdateFirebaseExtension, 0, FinalizeFirebaseExtension)
