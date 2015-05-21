/* $Id$ */

#ifndef MEMTRIGGER_H
#define MEMTRIGGER_H


#define MEMTRIGGER_ACTION_LEAVE_ACTIVE 0
#define MEMTRIGGER_ACTION_DISABLE 1


#define MEMTRIGGER_STATE_ACTIVE 0
#define MEMTRIGGER_STATE_DISABLED 1

#define MEMTRIGGER_TYPE_ABOVE 0
#define MEMTRIGGER_TYPE_BELOW 1


#ifdef ZEND_ENGINE_3
	#define MEMTRIGGER_ZEND_OBJECT zend_object
#else 
	#define MEMTRIGGER_ZEND_OBJECT void
#endif


#ifdef ZEND_ENGINE_3
/* Structure for MemTrigger object. */
typedef struct _php_memtrigger_object  {
	long calling;
	zval *callable;
	long value;
	long action;
	long state;
	long type;
	zend_object zo;
} php_memtrigger_object;
#else
/* Structure for Trigger object. */
typedef struct _php_memtrigger_object  {
	zend_object zo;
	long calling;
	zval *callable;
	long value;
	long action;
	long state;
	long type;
	zval *zself;//we store the zval that contains this object, to avoid having to retrieve it
} php_memtrigger_object;
#endif

//Object fetching.
#ifdef ZEND_ENGINE_3 
static inline php_memtrigger_object *php_memtrigger_fetch_object(zend_object *obj) {
	return (php_memtrigger_object *)((char*)(obj) - XtOffsetOf(php_memtrigger_object, zo));
}
#else 
#define php_memtrigger_fetch_object(object) ((php_memtrigger_object *)object)
#endif

// Object access
#ifdef ZEND_ENGINE_3
	#define Z_MEMTRIGGER_P(zv) php_memtrigger_fetch_object(Z_OBJ_P((zv)))
#else
	#define Z_MEMTRIGGER_P(zv) (php_memtrigger_object *)zend_object_store_get_object(zv TSRMLS_CC)
#endif


PHP_METHOD(memtrigger, __construct);
PHP_METHOD(memtrigger, setValue);
PHP_METHOD(memtrigger, getValue);
PHP_METHOD(memtrigger, setState);
PHP_METHOD(memtrigger, getState);
PHP_METHOD(memtrigger, setAction);
PHP_METHOD(memtrigger, getAction);


#define PHP_MEMTRIGGER_SC_NAME "MemTrigger"
#define PHP_MEMTRIGGER_EXCEPTION_SC_NAME "MemTriggerException"


#endif
