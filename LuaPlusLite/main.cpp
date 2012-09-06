//
//  main.cpp
//  LuaPlusLite
//
//  Created by David Ludwig on 9/5/12.
//  Copyright (c) 2012 David Ludwig. All rights reserved.
//

#include <assert.h>
#include <stdlib.h>

#include <iostream>
#include <string>
using namespace std;

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

namespace LuaPlusLite {
	static const char * LUAPLUSLITE_LUASTATE_REGISTRYSTRING = "LuaPlusLite_LuaState";

	class LuaState {
	public:
		LuaState() : c_state_(NULL) {
			c_state_ = luaL_newstate();
			lua_pushstring(c_state_, LUAPLUSLITE_LUASTATE_REGISTRYSTRING);
			lua_pushlightuserdata(c_state_, this);
			lua_settable(c_state_, LUA_REGISTRYINDEX);
		}
		
		~LuaState() {
			if (c_state_) {
				lua_close(c_state_);
				c_state_ = NULL;
			}
		}
		
		static LuaState * CastState(lua_State * wrapped_c_state) {
			if (wrapped_c_state == NULL) {
				return NULL;
			}
			lua_pushstring(wrapped_c_state, LUAPLUSLITE_LUASTATE_REGISTRYSTRING);
			lua_gettable(wrapped_c_state, LUA_REGISTRYINDEX);
			if ( ! lua_islightuserdata(wrapped_c_state, -1)) {
				lua_pop(wrapped_c_state, 1);
				return NULL;
			} else {
				LuaState * cpp_state = static_cast<LuaState*>(lua_touserdata(wrapped_c_state, -1));
				lua_pop(wrapped_c_state, 1);
				return cpp_state;
			}
		}
		
		lua_State * GetCState() const {
			return c_state_;
		}

	private:
		lua_State * c_state_;
	};

	class LuaObject {
	public:
		LuaObject() : lua_state_(NULL), ref_(LUA_NOREF) {
		}
		
		void Reset() {
			if (ref_ != LUA_NOREF && lua_state_ != NULL) {
				luaL_unref(lua_state_->GetCState(), LUA_REGISTRYINDEX, ref_);
				ref_ = LUA_NOREF;
				lua_state_ = NULL;
			}
		}

		// TODO: make sure that assigning a value to itself doesn't make the
		// old value get garbage collected
	
		void AssignInteger(LuaState * state, lua_Integer value) {
			AssignToState(state);
			lua_pushinteger(state->GetCState(), value);
			ref_ = luaL_ref(state->GetCState(), LUA_REGISTRYINDEX);
		}
		
		void AssignString(LuaState * state, const char * value) {
			AssignToState(state);
			lua_pushstring(state->GetCState(), value);
			ref_ = luaL_ref(state->GetCState(), LUA_REGISTRYINDEX);
		}
		
		void AssignNil(LuaState * state) {
			AssignToState(state);
			lua_pushnil(state->GetCState());
			ref_ = luaL_ref(state->GetCState(), LUA_REGISTRYINDEX);
		}
		
		void AssignNewTable(LuaState * state, int narr = 0, int nrec = 0) {
			AssignToState(state);
			lua_createtable(state->GetCState(), narr, nrec);
			ref_ = luaL_ref(state->GetCState(), LUA_REGISTRYINDEX);
		}
		
		int Type() {
			// TODO: check validity of object state, and type of value
			lua_rawgeti(lua_state_->GetCState(), LUA_REGISTRYINDEX, ref_);
			int type = lua_type(lua_state_->GetCState(), -1);
			lua_pop(lua_state_->GetCState(), 1);
			return type;
		}
		
		const char * TypeName() {
			// TODO: check validity of object state, and type of value
			int type = Type();
			return lua_typename(lua_state_->GetCState(), type);
		}
		
		lua_Integer ToInteger(int * isnum = NULL) {
			// TODO: check validity of object state, and type of value
			lua_rawgeti(lua_state_->GetCState(), LUA_REGISTRYINDEX, ref_);
			lua_Integer value = lua_tointegerx(lua_state_->GetCState(), -1, isnum);
			lua_pop(lua_state_->GetCState(), 1);
			return value;
		}
		
		const char * ToString(size_t * len = NULL) {
			// TODO: check validity of object state, and type of value
			lua_rawgeti(lua_state_->GetCState(), LUA_REGISTRYINDEX, ref_);
			const char * value = lua_tolstring(lua_state_->GetCState(), -1, len);
			lua_pop(lua_state_->GetCState(), 1);
			return value;
		}
		
	private:
		void AssignToState(LuaState * state) {
			// TODO: check validity of state and consider throwing a LuaException if it is invalid
			// TODO: consider returning early here if 'lua_state_ == state'
			Reset();
			// TODO: see if lua_state_ is/should-be reference counted
			lua_state_ = state;
		}
	
		LuaState * lua_state_;
		int ref_;
	};

	class LuaStackObject {
	public:
	};
}
using namespace LuaPlusLite;

int main(int argc, const char * argv[])
{
	cout << "Welcome to LuaPlusLite!\n";
	srand(0);
	
	cout << "Making a LuaState\n";
	LuaState myLuaState;
	
	lua_State * myLuaState_CState = myLuaState.GetCState();
	cout << "... Done!\n";
	cout << "... Inner lua_State is " << myLuaState_CState << endl;
	cout << "... Wrapper (a LuaState) is at " << &myLuaState << endl;
	assert(myLuaState_CState != NULL);
	
	cout << "Retrieving the C++ LuaState via the wrapped, C-based, lua_State\n";
	LuaState * luaStateFromInnerState = LuaState::CastState(myLuaState_CState);
	cout << "... LuaState from inner, C-based, lua_State: " << luaStateFromInnerState << endl;
	assert(luaStateFromInnerState == &myLuaState);
	
	cout << "Assigning and retrieving a random integer: ";
	lua_Integer random_number = rand();
	cout << random_number << endl;
	LuaObject myEncodedInteger;
	myEncodedInteger.AssignInteger(&myLuaState, random_number);
	cout << "... encoded type number is " << myEncodedInteger.Type() << endl;
	assert(myEncodedInteger.Type() == LUA_TNUMBER);
	cout << "... encoded type name " << myEncodedInteger.TypeName() << endl;
	assert(strcmp(myEncodedInteger.TypeName(), "number") == 0);
	lua_Integer decoded_number = myEncodedInteger.ToInteger();
	cout << "... decoded number is " << decoded_number << endl;
	assert(random_number == decoded_number);
	
	cout << "Assigning and retrieving a random string: ";
	char random_string[16];
	const size_t num_chars_in_random_string = sizeof(random_string) / sizeof(char);
	for (int i = 0; i < (num_chars_in_random_string - 1); i++) {
		int ch = rand() % 26;
		ch += 'A';
		random_string[i] = ch;
	}
	random_string[num_chars_in_random_string - 1] = 0;
	cout << random_string << endl;
	LuaObject myEncodedString;
	myEncodedString.AssignString(&myLuaState, random_string);
	cout << "... encoded type number is " << myEncodedString.Type() << endl;
	assert(myEncodedString.Type() == LUA_TSTRING);
	cout << "... encoded type name " << myEncodedString.TypeName() << endl;
	assert(strcmp(myEncodedString.TypeName(), "string") == 0);
	const char * decoded_string = myEncodedString.ToString();
	cout << "... decoded string is " << decoded_string << endl;
	assert(decoded_string != NULL);
	assert(strcmp(random_string, decoded_string) == 0);
	
	cout << "Assigning and retrieving a nil value\n";
	LuaObject myNilObject;
	myNilObject.AssignNil(&myLuaState);
	cout << "... encoded type number is " << myNilObject.Type() << endl;
	assert(myNilObject.Type() == LUA_TNIL);
	cout << "... encoded type name " << myNilObject.TypeName() << endl;
	assert(strcmp(myNilObject.TypeName(), "nil") == 0);
	
	cout << "Assigning a new table\n";
	LuaObject myTable;
	myTable.AssignNewTable(&myLuaState);
	cout << "... encoded type number is " << myTable.Type() << endl;
	assert(myTable.Type() == LUA_TTABLE);
	cout << "... encoded type name " << myTable.TypeName() << endl;
	assert(strcmp(myTable.TypeName(), "table") == 0);
	
    return 0;
}
