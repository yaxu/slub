/usr/bin/perl /usr/lib/perl5/5.8.5/ExtUtils/xsubpp  -typemap /usr/lib/perl5/5.8.5/ExtUtils/typemap   test_pl_0d61.xs > test_pl_0d61.xsc && mv test_pl_0d61.xsc test_pl_0d61.c
gcc -c  -I/music/new/inline/ -D_REENTRANT -D_GNU_SOURCE -DTHREADS_HAVE_PIDS -DDEBUGGING -fno-strict-aliasing -pipe -I/usr/local/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -I/usr/include/gdbm -O2 -g -pipe -m32 -march=i386 -mtune=pentium4   -DVERSION=\"0.00\" -DXS_VERSION=\"0.00\" -fPIC "-I/usr/lib/perl5/5.8.5/i386-linux-thread-multi/CORE"   test_pl_0d61.c
test_pl_0d61.c:19: error: syntax error before "void"
make: *** [test_pl_0d61.o] Error 1
