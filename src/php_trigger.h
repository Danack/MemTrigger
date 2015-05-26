/* $Id$ */

#ifndef PHP_TRIGGER_H
#define PHP_TRIGGER_H

extern zend_module_entry trigger_module_entry;


#include "php_version.h"
#define PHP_TRIGGER_VERSION PHP_VERSION

extern zend_class_entry *php_trigger_sc_entry;
extern zend_class_entry *php_trigger_exception_class_entry;

PHP_MINIT_FUNCTION(trigger);
PHP_MSHUTDOWN_FUNCTION(trigger);
PHP_RINIT_FUNCTION(trigger);
PHP_RSHUTDOWN_FUNCTION(trigger);
PHP_MINFO_FUNCTION(trigger);

PHP_FUNCTION(trigger_init);
PHP_FUNCTION(trigger_register);

ZEND_BEGIN_MODULE_GLOBALS(trigger)
	long initialized;
	zend_bool enabled;
	HashTable *user_triggers;
	long ticks_between_mem_check;
	long ticks_till_next_mem_check;
	long trigger_shutting_down;
	char *implementation_method;
ZEND_END_MODULE_GLOBALS(trigger)

#ifdef ZTS
#define TRIGGER_G(v) TSRMG(trigger_globals_id, zend_trigger_globals *, v)
#else
#define TRIGGER_G(v)	(trigger_globals.v)
#endif

#endif	/* PHP_TRIGGER_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
