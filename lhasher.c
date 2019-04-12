//#include "blake2b.h"

#include <lua.h>
#include <lauxlib.h>
#include "src/hashs.h"

#define VERSION	"hasher 0.1.0"

/* compability with lua 5.1 */
#if LUA_VERSION_NUM <= 501
#define luaL_setfuncs(L,r,n) \
	luaL_register(L,NULL,r)
#endif

static int lblake2b(lua_State *L) {
	// compute the hash of a string
	// lua api:  blake2b(m [, digln [, key]]) return digest
	// m: the string to be hashed
	// digln: the optional length of the digest to be computed
	// (between 1 and 64) - default value is 64
	// key: an optional secret key, allowing blake2b to work as a MAC
	//    (if provided, key length must be between 1 and 64)
	//    default is no key
	// digest: the blake2b hash as a string (string length is digln,
	// so default hash is a 64-byte string)
	size_t mln;
	size_t keyln = 0;
	const char *m = luaL_checklstring(L, 1, &mln);
	int digln = luaL_optinteger(L, 2, 64);
	const char *key = luaL_optlstring(L, 3, NULL, &keyln);
	if(keyln > 64)
    luaL_error(L, "bad key size");
	if(digln < 1 || digln > 64)
    luaL_error(L, "bad digest size");
	char digest[64];
	blake2b(
    (uint8_t*)digest, digln,
    (const uint8_t*)key, keyln,
    (const uint8_t*)m, mln);
	lua_pushlstring(L, digest, digln);
	return 1;
}

static int lbase58_encode(lua_State *L) {
	// lua api:  b58encode(str) => encoded | (nil, error msg)
	// prereq:  #str <= 256  (defined as BASE58_ENCODE_MAXLEN)
	size_t bln, eln;
	char buf[BASE58_DECODE_MAXLEN]; 	// buffer to receive encoded string
	const char *b = luaL_checklstring(L,1,&bln);
	if (bln == 0) { // empty string special case (not ok with b58enc)
		lua_pushliteral(L, "");
		return 1;
	} else if (bln > BASE58_ENCODE_MAXLEN) {
		luaL_error(L, "string too long");
	}
	eln = BASE58_DECODE_MAXLEN; // eln must be set to buffer size before calling b58enc
	bool r = base58_encode(buf, &eln, b, bln);
	if(!r)
		luaL_error(L, "base58 encode error");
	eln = eln - 1;  // b58enc add \0 at the end of the encode string
	lua_pushlstring(L, buf, eln);
	return 1;
}

static int lbase58_decode(lua_State *L) {
	// lua api: b58decode(encstr) => str | (nil, error msg)
	size_t bln, eln;
	char buf[BASE58_DECODE_MAXLEN]; 	// buffer to receive decoded string
	const char *e = luaL_checklstring(L,1,&eln); // encoded data
	if (eln == 0) { // empty string special case
		lua_pushliteral(L, "");
		return 1;
	} else if (eln > BASE58_DECODE_MAXLEN) {
		lua_pushnil(L);
		lua_pushfstring(L, "string too long");
		return 2;
	}
	bln = BASE58_DECODE_MAXLEN;
	bool r = base58_decode(buf, &bln, e, eln);
	if (!r) {
		lua_pushnil(L);
		lua_pushfstring(L, "b58decode error");
		return 2;
	}
	// base58_decode returns its result at the end of buf!!!
	lua_pushlstring(L, buf + BASE58_DECODE_MAXLEN - bln, bln);
	return 1;
}

static const luaL_Reg R[] = {
	{ "blake2b",	lblake2b },
	{ "base58encode",	lbase58_encode },
	{ "base58decode",	lbase58_decode },
	{ NULL,		NULL	}
};

LUALIB_API int luaopen_hasher(lua_State *L)
{
  lua_newtable(L);
  luaL_setfuncs(L,R,0);
  lua_pushliteral(L, "VERSION");
  lua_pushliteral(L, VERSION);
  lua_settable(L, -3);
  return 1;
}
