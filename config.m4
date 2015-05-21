dnl
dnl $Id$
dnl

PHP_ARG_ENABLE(memtrigger, whether to enable memory triggers support,
[  --enable-memtrigger          Enable memory trigger support ])

if test "$PHP_MSL" != "no"; then
  PHP_NEW_EXTENSION(memtrigger, src/php_memtrigger.c src/memtrigger.c, $ext_shared, )
fi
