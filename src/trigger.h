/* $Id$ */

#ifndef TRIGGER_H
#define TRIGGER_H


//#define DIRECTIONAL_TRIGGERS

#define TRIGGER_ACTION_LEAVE_ACTIVE 0
#define TRIGGER_ACTION_DISABLE 1

#define TRIGGER_STATE_ACTIVE 0
#define TRIGGER_STATE_DISABLED 1

#ifdef DIRECTIONAL_TRIGGERS
#define TRIGGER_TYPE_ABOVE 0
#define TRIGGER_TYPE_BELOW 1
#endif


#ifdef ZEND_ENGINE_3
	#define TRIGGER_ZEND_OBJECT zend_object
#else 
	#define TRIGGER_ZEND_OBJECT void
#endif


#ifdef ZEND_ENGINE_3
/* Structure for Trigger object. */
typedef struct _php_trigger_object  {
	long calling;
	zval callable;
	long value;
	long action;
	long state;
#ifdef DIRECTIONAL_TRIGGERS
	long type;
#endif
	zval *zself;//we store the zval that contains this object, to avoid having to retrieve it
	zend_object zo;
} php_trigger_object;
#else
/* Structure for Trigger object. */
typedef struct _php_trigger_object  {
	zend_object zo;
	long calling;
	zval *callable;
	long value;
	long action;
	long state;
#ifdef DIRECTIONAL_TRIGGERS
	long type;
#endif
	zval *zself;//we store the zval that contains this object, to avoid having to retrieve it
} php_trigger_object;
#endif

//Object fetching.
#ifdef ZEND_ENGINE_3 
static inline php_trigger_object *php_trigger_fetch_object(zend_object *obj) {
	return (php_trigger_object *)((char*)(obj) - XtOffsetOf(php_trigger_object, zo));
}
#else 
#define php_trigger_fetch_object(object) ((php_trigger_object *)object)
#endif

// Object access
#ifdef ZEND_ENGINE_3
	#define Z_TRIGGER_P(zv) php_trigger_fetch_object(Z_OBJ_P((zv)))
#else
	#define Z_TRIGGER_P(zv) (php_trigger_object *)zend_object_store_get_object(zv TSRMLS_CC)
#endif


PHP_METHOD(trigger, __construct);
PHP_METHOD(trigger, setValue);
PHP_METHOD(trigger, getValue);
PHP_METHOD(trigger, setState);
PHP_METHOD(trigger, getState);
PHP_METHOD(trigger, setAction);
PHP_METHOD(trigger, getAction);


#define PHP_TRIGGER_SC_NAME "Trigger"
#define PHP_TRIGGER_EXCEPTION_SC_NAME "TriggerException"

#ifdef ZEND_ENGINE_3
	#define TRIGGER_LEN_TYPE size_t
#else
	#define TRIGGER int
#endif


#endif
