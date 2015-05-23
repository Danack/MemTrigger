
/* $Id$ */

//#include "TSRM.h"
//#include "Zend/zend.h"

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_memtrigger.h"

#include "memtrigger.h"
#include "php_ticks.h"

#include "zend.h"
#include "zend_compile.h"
#include "zend_execute.h"
#include "zend_exceptions.h"
#include "zend_gc.h"
#include "zend_API.h"
#include "ext/spl/spl_exceptions.h"



/* {{{ proto MemTrigger::__construct()
   
*/
PHP_METHOD(memtrigger, __construct)
{
	zval *callback;
	long value = 0;
	long action = 0;
	long type = MEMTRIGGER_STATE_ACTIVE;
	long state = MEMTRIGGER_TYPE_ABOVE;

	php_memtrigger_object *memtrigger;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zll|ll", &callback, &value, &action, &type, &state) == FAILURE) {
		return;
	}

	// Check whether the callback is valid now, rather than failing later
	if (!callback || !zend_is_callable(callback, 0, NULL TSRMLS_CC)) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC,
			"Invalid argument, callback must be a callable.");
		RETURN_FALSE;
	}

	memtrigger = Z_MEMTRIGGER_P(getThis());

#ifdef ZEND_ENGINE_3
	Z_TRY_ADDREF_P(callback);
	ZVAL_COPY_VALUE(&memtrigger->callable, callback);
#else
	Z_ADDREF_P(callback);
	memtrigger->callable = callback;
#endif

	memtrigger->calling = 0;
	memtrigger->value = value;
	memtrigger->action = action;
	memtrigger->state = state;
	memtrigger->type = type;
	memtrigger->zself = getThis();
}
/* }}} */


/* {{{ proto bool Imagick::setFilename(string filename)
	Sets the filename before you read or write an image file.
*/
PHP_METHOD(memtrigger, setValue)
{
	php_memtrigger_object *memtrigger;
	long newValue;

	/* Parse parameters given to function */
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &newValue) == FAILURE) {
		return;
	}

	memtrigger = Z_MEMTRIGGER_P(getThis());
	memtrigger->value = newValue;

	RETURN_TRUE;
}
/* }}} */


/* {{{ proto bool MemTrigger::getTriggerValue()
*/
PHP_METHOD(memtrigger, getValue)
{
	php_memtrigger_object *memtrigger;
	memtrigger = Z_MEMTRIGGER_P(getThis());

	RETURN_LONG(memtrigger->value);
}
/* }}} */


/* {{{ proto bool MemTrigger::setState($state)
*/
PHP_METHOD(memtrigger, setState)
{
	php_memtrigger_object *memtrigger;
	long newState;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &newState) == FAILURE) {
		return;
	}

	memtrigger = Z_MEMTRIGGER_P(getThis());
	memtrigger->state = newState;
}
/* }}} */



/* {{{ proto bool MemTrigger::getState()
*/
PHP_METHOD(memtrigger, getState)
{
	php_memtrigger_object *memtrigger;
	memtrigger = Z_MEMTRIGGER_P(getThis());

	RETURN_LONG(memtrigger->state);
}
/* }}} */


/* {{{ proto bool MemTrigger::setTriggerAction($action)
*/
PHP_METHOD(memtrigger, setAction)
{

	php_memtrigger_object *memtrigger;
	long newAction;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &newAction) == FAILURE) {
		return;
	}

	memtrigger = Z_MEMTRIGGER_P(getThis());
	memtrigger->action = newAction;
}
/* }}} */



/* {{{ proto bool MemTrigger::getTriggerAction()
*/
PHP_METHOD(memtrigger, getAction)
{
	php_memtrigger_object *memtrigger;
	memtrigger = Z_MEMTRIGGER_P(getThis());

	RETURN_LONG(memtrigger->action);

}
/* }}} */


void php_memtrigger_initialize_constants(TSRMLS_D)
{
#define MT_REGISTER_CONST_LONG(const_name, value)\
	zend_declare_class_constant_long(php_memtrigger_sc_entry, const_name, sizeof(const_name)-1, (long)value TSRMLS_CC);

#define  MT_REGISTER_CONST_STRING(const_name, value)\
	zend_declare_class_constant_string(php_memtrigger_sc_entry, const_name, sizeof(const_name)-1, value TSRMLS_CC);

	/* Constants */
	MT_REGISTER_CONST_LONG("ACTION_LEAVE_ACTIVE",  MEMTRIGGER_ACTION_LEAVE_ACTIVE);
	MT_REGISTER_CONST_LONG("ACTION_DISABLE",  MEMTRIGGER_ACTION_DISABLE);

	MT_REGISTER_CONST_LONG("STATE_ACTIVE",  MEMTRIGGER_STATE_ACTIVE);
	MT_REGISTER_CONST_LONG("STATE_DISABLED",  MEMTRIGGER_STATE_DISABLED);

	MT_REGISTER_CONST_LONG("TYPE_ABOVE",  MEMTRIGGER_TYPE_ABOVE);
	MT_REGISTER_CONST_LONG("TYPE_BELOW",  MEMTRIGGER_TYPE_BELOW);

	//IMAGICK_REGISTER_CONST_STRING("IMAGICK_EXTVER", PHP_IMAGICK_VERSION);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
