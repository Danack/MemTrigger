dnl
dnl $Id$
dnl

PHP_ARG_ENABLE(trigger, whether to enable triggers support,
[  --enable-trigger          Enable trigger support no, yes, opcode, mem])

if test "$PHP_TRIGGER" != "no"; then
  PHP_NEW_EXTENSION(trigger, src/php_trigger.c src/trigger.c, $ext_shared, )
fi

dnl AC_MSG_CHECKING([Checking PHP version])
dnl if test -z "${PHP_VERSION_ID}"; then
dnl   if test -z "${PHP_CONFIG}"; then
dnl     AC_MSG_ERROR([php-config not found])
dnl   fi
dnl   if test -z "${AWK}"; then
dnl     AC_MSG_ERROR([awk not found])
dnl   fi
dnl   PHP_TRIGGER_FOUND_VERSION=`${PHP_CONFIG} --version`
dnl   PHP_TRIGGER_FOUND_VERNUM=`echo "${PHP_TRIGGER_FOUND_VERSION}" | $AWK 'BEGIN { FS = "."; } { printf "%d", ([$]1 * 100 + [$]2) * 100 + [$]3;}'`
dnl else
dnl   PHP_TRIGGER_FOUND_VERNUM="${PHP_VERSION_ID}"
dnl   PHP_TRIGGER_FOUND_VERSION="${PHP_VERSION}"
dnl fi

dnl PHP_TRIGGER_IMPL="OPCODE"

dnl if test "$PHP_TRIGGER_FOUND_VERNUM" -ge "70000"; then
dnl   AC_MSG_RESULT(Found $PHP_TRIGGER_FOUND_VERSION $PHP_TRIGGER_FOUND_VERNUM both opcode and mem triggers available)
dnl   PHP_TRIGGER_IMPL="MEM"
dnl else 
dnl   AC_MSG_RESULT(Found $PHP_TRIGGER_FOUND_VERSION $PHP_TRIGGER_FOUND_VERNUM only opcode triggers available, mem hooks unavailble)
dnl fi
dnl 
dnl 
dnl 
dnl AC_MSG_RESULT(Triggers will use $PHP_TRIGGER_IMPL implementation)
dnl 
dnl 
dnl if test "$PHP_TRIGGER_IMPL" == "OPCODE"; then
dnl     AC_DEFINE_UNQUOTED([TRIGGER_IMPLEMENTATION_OPCODE], 1, [Use OPCODE trigger implementation])
dnl elif test "$PHP_TRIGGER_IMPL" == "MEM"; then
dnl     AC_DEFINE_UNQUOTED([TRIGGER_IMPLEMENTATION_MEM], 1, [Use OPCODE trigger implementation])
dnl else
dnl     AC_DEFINE_UNQUOTED([TRIGGER_IMPLEMENTATION_UNKNOWN], 1, [Use unknown trigger implementation, aka config is borked])
dnl fi

