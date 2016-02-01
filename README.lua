SimGrid requires Lua 5.3; it will not work with Lua 5.2 or Lua 5.1,
as Lua 5.3 breaks backwards compatibility. 
Version 5.3.2, 5.3.3 or any 5.3.X are ok, though.

However, installing Lua 5.3 is easy. (If you are an administrator of your machine)

Step 1: Go to http://www.lua.org/download.html and download the 5.3 package.

Step 2: Untar the package: tar xvfz lua-5.3.*.tar.gz

Step 3: cd into the new directory

Step 4: Apply the patch in "<simgrid-source-dir>/tool/lualib.patch" to the
        lua source:

        For instance, if you unpacked the lua sourcecode to /tmp/lua-5.3.1, use
        the following commands:

        cp tools/lualib.patch /tmp/lua-5.3.1
        cd /tmp/lua-5.3.1/
        patch -p1 < lualib.patch

Step 5: make linux

Step 6: sudo make install

Step 7: Go back to the SimGrid source, and run ccmake again. Try removing CMakeCache.txt if it still complains about Lua being not found.
