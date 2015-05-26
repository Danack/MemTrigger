
/* $Id$ */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_trigger.h"

#include "trigger.h"
#include "php_ticks.h"

#include "zend.h"
#include "zend_compile.h"
#include "zend_execute.h"
#include "zend_exceptions.h"
#include "zend_gc.h"
#include "zend_API.h"
#include "ext/spl/spl_exceptions.h"


/* {{{ proto Trigger::__construct()
*/
PHP_METHOD(trigger, __construct)
{
	zval *callback;
	long value = 0;
	long action = 0;
#ifdef DIRECTIONAL_TRIGGERS
	long type = TRIGGER_TYPE_ABOVE;
#endif
	long state = TRIGGER_STATE_ACTIVE ;

	php_trigger_object *trigger;

#ifdef DIRECTIONAL_TRIGGERS
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zll|ll", &callback, &value, &action, &type, &state) == FAILURE) {
		return;
	}
#else
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zll|ll", &callback, &value, &action, &state) == FAILURE) {
		return;
	}
#endif

	// Check whether the callback is valid now, rather than failing later
	if (!callback || !zend_is_callable(callback, 0, NULL TSRMLS_CC)) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC,
			"Invalid argument, callback must be a callable.");
		RETURN_FALSE;
	}

	trigger = Z_TRIGGER_P(getThis());

#ifdef ZEND_ENGINE_3
	Z_TRY_ADDREF_P(callback);
	ZVAL_COPY_VALUE(&trigger->callable, callback);
#else
	Z_ADDREF_P(callback);
	trigger->callable = callback;
#endif

	trigger->calling = 0;
	trigger->value = value;
	trigger->action = action;
	trigger->state = state;
#ifdef DIRECTIONAL_TRIGGERS
	trigger->type = type;
#endif
	trigger->zself = getThis();
}
/* }}} */


/* {{{ proto bool Imagick::setFilename(string filename)
	Sets the filename before you read or write an image file.
*/
PHP_METHOD(trigger, setValue)
{
	php_trigger_object *trigger;
	long newValue;

	/* Parse parameters given to function */
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &newValue) == FAILURE) {
		return;
	}

	trigger = Z_TRIGGER_P(getThis());
	trigger->value = newValue;

	RETURN_TRUE;
}
/* }}} */


/* {{{ proto bool Trigger::getTriggerValue()
*/
PHP_METHOD(trigger, getValue)
{
	php_trigger_object *trigger;
	trigger = Z_TRIGGER_P(getThis());

	RETURN_LONG(trigger->value);
}
/* }}} */


/* {{{ proto bool Trigger::setState($state)
*/
PHP_METHOD(trigger, setState)
{
	php_trigger_object *trigger;
	long newState;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &newState) == FAILURE) {
		return;
	}

	trigger = Z_TRIGGER_P(getThis());
	trigger->state = newState;
}
/* }}} */



/* {{{ proto bool Trigger::getState()
*/
PHP_METHOD(trigger, getState)
{
	php_trigger_object *trigger;
	trigger = Z_TRIGGER_P(getThis());

	RETURN_LONG(trigger->state);
}
/* }}} */


/* {{{ proto bool Trigger::setTriggerAction($action)
*/
PHP_METHOD(trigger, setAction)
{

	php_trigger_object *trigger;
	long newAction;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &newAction) == FAILURE) {
		return;
	}

	trigger = Z_TRIGGER_P(getThis());
	trigger->action = newAction;
}
/* }}} */



/* {{{ proto bool Trigger::getTriggerAction()
*/
PHP_METHOD(trigger, getAction)
{
	php_trigger_object *trigger;
	trigger = Z_TRIGGER_P(getThis());

	RETURN_LONG(trigger->action);
}
/* }}} */


void php_trigger_initialize_constants(TSRMLS_D)
{
#define MT_REGISTER_CONST_LONG(const_name, value)\
	zend_declare_class_constant_long(php_trigger_sc_entry, const_name, sizeof(const_name)-1, (long)value TSRMLS_CC);

#define  MT_REGISTER_CONST_STRING(const_name, value)\
	zend_declare_class_constant_string(php_trigger_sc_entry, const_name, sizeof(const_name)-1, value TSRMLS_CC);

	MT_REGISTER_CONST_LONG("ACTION_LEAVE_ACTIVE",  TRIGGER_ACTION_LEAVE_ACTIVE);
	MT_REGISTER_CONST_LONG("ACTION_DISABLE",  TRIGGER_ACTION_DISABLE);

	MT_REGISTER_CONST_LONG("STATE_ACTIVE",  TRIGGER_STATE_ACTIVE);
	MT_REGISTER_CONST_LONG("STATE_DISABLED",  TRIGGER_STATE_DISABLED);

#if DIRECTIONAL_TRIGGERS
	MT_REGISTER_CONST_LONG("TYPE_ABOVE",  TRIGGER_TYPE_ABOVE);
	MT_REGISTER_CONST_LONG("TYPE_BELOW",  TRIGGER_TYPE_BELOW);
#endif
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
