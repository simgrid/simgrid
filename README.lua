SimGrid requires Lua 5.3; it will not work with Lua 5.2 or Lua 5.1,
as Lua 5.3 breaks backwards compatibility.

However, installing Lua 5.3 is easy. (If you're an administrator)

Step 1: Go to http://www.lua.org/download.html and download the 5.3 package.

Step 2: Untar the package: tar xvfz lua-5.3.*.tar.gz

Step 3: cd into the new directory

Step 4: Apply the patch in "<simgrid-source-dir>/contrib/lua/lualib.patch" to the
        lua source:

        For instance, if you unpacked the lua sourcecode to /tmp/lua-5.3.1, use
        the following commands:

        cp contrib/lua/lualib.patch /tmp/lua-5.3.1
        cd /tmp/lua-5.3.1/
        patch -p1 < lualib.patch

Step 5: make <platform>, for instance "make linux"

Step 6: sudo make install

Step 7: Run ccmake (or supply the config option to cmake) to enable Lua in SimGrid. Done!
