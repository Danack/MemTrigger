/* $Id$ */

#ifndef PHP_MEMTRIGGER_H
#define PHP_MEMTRIGGER_H

extern zend_module_entry memtrigger_module_entry;


#include "php_version.h"
#define PHP_MEMTRIGGER_VERSION PHP_VERSION

PHP_MINIT_FUNCTION(memtrigger);
PHP_MSHUTDOWN_FUNCTION(memtrigger);
PHP_RINIT_FUNCTION(memtrigger);
PHP_RSHUTDOWN_FUNCTION(memtrigger);
PHP_MINFO_FUNCTION(memtrigger);

PHP_FUNCTION(memtrigger_register);
PHP_FUNCTION(memtrigger_init);

ZEND_BEGIN_MODULE_GLOBALS(memtrigger)
	zend_llist *user_tick_functions;
	long ticks_between_mem_check;
	long ticks_till_next_mem_check;
ZEND_END_MODULE_GLOBALS(memtrigger)

#ifdef ZTS
#define MEMTRIGGER_G(v) TSRMG(memtrigger_globals_id, zend_memtrigger_globals *, v)
#else
#define MEMTRIGGER_G(v)	(memtrigger_globals.v)
#endif

#endif	/* PHP_MEMTRIGGER_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
