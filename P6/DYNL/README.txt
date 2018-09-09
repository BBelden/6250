You need to set the env var LD_LIBRARY_PATH to . (dot) to 
run the dll versions.

You can use 'ldd -v fn' on file to see the list of so's that it uses.
You can also use nm to see the symbols.  For example, if you use nm on
statichello, it shows print_hello as a 'T' symbol (in the text section of
the code).  Also, ldd fails on statichello because it is totally static
(all called stuff including libc is included); it is quite large.  But,
if you use nm on dlinkhello (same as statichello except dynamically
linked), it shows print_hello to be a 'U' type symbol (undefined).

However, if you build statichello this way:
    gcc -g -o statichello statichello.o libhello.a
nm shows print_hello as in the Text.  But, statichello is now a
dynamically linked program for all the other stuff, e.g. libc.
