tmp_src_filename=fdht_check_bits.c
cat <<EOF > $tmp_src_filename
#include <stdio.h>
int main()
{
	printf("%d\n", sizeof(long));
	return 0;
}
EOF

gcc -D_FILE_OFFSET_BITS=64 -o a.out $tmp_src_filename
bytes=`./a.out`

/bin/rm -f  a.out $tmp_src_filename
if [ "$bytes" -eq 8 ]; then
 OS_BITS=64
else
 OS_BITS=32
fi

cat <<EOF > common/_os_bits.h
#ifndef _OS_BITS_H
#define _OS_BITS_H

#define OS_BITS  $OS_BITS

#endif
EOF

#WITH_LINUX_SERVICE=1

TARGET_PREFIX=/usr/local

CFLAGS='-O3 -Wall -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE'
#CFLAGS='-g -Wall -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D__DEBUG__ -I /home/o/o/happyfish100/libevent/include -L /home/o/o/happyfish100/libevent/lib'

LIBS=''
uname=`uname`
if [ "$uname" = "Linux" ]; then
  CFLAGS="$CFLAGS -DOS_LINUX"
elif [ "$uname" = "FreeBSD" ]; then
  CFLAGS="$CFLAGS -DOS_FREEBSD"
elif [ "$uname" = "SunOS" ]; then
  CFLAGS="$CFLAGS -DOS_SUNOS"
  LIBS="$LIBS -lsocket -lnsl -lresolv"
  export CC=gcc
elif [ "$uname" = "AIX" ]; then
  CFLAGS="$CFLAGS -DOS_AIX"
  export CC=gcc
fi

if [ -f /usr/lib/libpthread.so ] || [ -f /usr/local/lib/libpthread.so ] || [ -f /usr/lib64/libpthread.so ] || [ -f /usr/lib/libpthread.a ] || [ -f /usr/local/lib/libpthread.a ] || [ -f /usr/lib64/libpthread.a ]; then
  LIBS="$LIBS -lpthread"
else
  line=`nm -D /usr/lib/libc_r.so | grep pthread_create | grep -w T`
  if [ -n "$line" ]; then
    LIBS="$LIBS -lc_r"
  fi
fi

cd server
cp Makefile.in Makefile
perl -pi -e "s#\\\$\(CFLAGS\)#$CFLAGS#g" Makefile
perl -pi -e "s#\\\$\(LIBS\)#$LIBS#g" Makefile
perl -pi -e "s#\\\$\(TARGET_PREFIX\)#$TARGET_PREFIX#g" Makefile
make $1 $2

cd ../tool 
cp Makefile.in Makefile
perl -pi -e "s#\\\$\(CFLAGS\)#$CFLAGS#g" Makefile
perl -pi -e "s#\\\$\(LIBS\)#$LIBS#g" Makefile
perl -pi -e "s#\\\$\(TARGET_PREFIX\)#$TARGET_PREFIX#g" Makefile
make $1 $2

cd ../client
cp Makefile.in Makefile
perl -pi -e "s#\\\$\(CFLAGS\)#$CFLAGS#g" Makefile
perl -pi -e "s#\\\$\(LIBS\)#$LIBS#g" Makefile
perl -pi -e "s#\\\$\(TARGET_PREFIX\)#$TARGET_PREFIX#g" Makefile
make $1 $2

#cd test
#cp Makefile.in Makefile
#perl -pi -e "s#\\\$\(CFLAGS\)#$CFLAGS#g" Makefile
#perl -pi -e "s#\\\$\(LIBS\)#$LIBS#g" Makefile
#perl -pi -e "s#\\\$\(TARGET_PREFIX\)#$TARGET_PREFIX#g" Makefile
#cd ..

if [ "$1" = "install" ]; then
  cd ..
  cp -f restart.sh $TARGET_PREFIX/bin
  cp -f stop.sh $TARGET_PREFIX/bin

  if [ "$uname" = "Linux" ]; then
    if [ "$WITH_LINUX_SERVICE" = "1" ]; then
      mkdir -p /etc/fdht
      cp -f conf/fdhtd.conf /etc/fdht/
      cp -f conf/fdht_servers.conf /etc/fdht/
      cp -f conf/fdht_client.conf /etc/fdht/
      cp -f init.d/fdhtd /etc/rc.d/init.d/
      /sbin/chkconfig --add fdhtd
    fi
  fi
fi

