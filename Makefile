LUAINC=/usr/local/include
LUAEXE=lua
CC=gcc -std=c99
CFLAGS=-Wall -Wextra -O2 -I$(LUAINC) -I./src/
LDFLAGS=-shared -fPIC
LIBNAME=hasher
SRCS=lhasher.c src/blake2b.c src/base58.c
ROCKSPEC=rocks/hasher-0.1.0-1.rockspec

all: $(LIBNAME).so

test: $(LIBNAME).so
	$(LUAEXE) test.lua

test-all:
	$(MAKE) clean
	$(MAKE) test
	$(MAKE) clean
	$(MAKE) LUAINC=/usr/include/luajit-2.1 LUAEXE=luajit test
	$(MAKE) clean
	$(MAKE) LUAINC=/usr/include/lua5.1 LUAEXE=lua5.1 test

test-rocks:
	luarocks make --lua-version=5.3 --local $(ROCKSPEC)
	luarocks make --lua-version=5.1 --local $(ROCKSPEC)
	lua5.1 test.lua
	lua5.3 test.lua
	luajit test.lua

upload-rocks:
	luarocks upload --api-key=$(LUAROCKS_APIKEY) $(ROCKSPEC)

clean:
	rm -f *.o *.so
	rm -f src/*.o

hasher.so: $(SRCS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(LIBNAME).so $(SRCS)

.PHONY: all test test-all test-rocks upload-rocks clean
