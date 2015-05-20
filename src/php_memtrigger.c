
/* $Id$ */

#define MEMTRIGGER_DEBUG 0

#if MEMTRIGGER_DEBUG
#define DEBUG_OUT printf("DEBUG: ");printf
#define IF_DEBUG(z) z
#else
#define IF_DEBUG(z)
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_memtrigger.h"
#include "php_ticks.h"
#include "zend_gc.h"
#include "ext/spl/spl_exceptions.h"



#include <errno.h>


typedef struct _memtrigger_tick_function_entry {
	int calling;
	zval *callable;
	long triggerBytes;
	long resetBytes;
	int triggered;
} memtrigger_tick_function_entry;


/* some prototypes for local functions */
static void memtrigger_tick_function_dtor(memtrigger_tick_function_entry *tick_function_entry);
static int memtrigger_tick_function_call(memtrigger_tick_function_entry *tick_fe, long memory_usage TSRMLS_DC);
static void run_memtrigger_tick_functions(int tick_count);


ZEND_DECLARE_MODULE_GLOBALS(memtrigger)
static PHP_GINIT_FUNCTION(memtrigger);

/* {{{ arginfo */
ZEND_BEGIN_ARG_INFO(arginfo_memtrigger_void, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_memtrigger_init, 0, 0, 1)
		ZEND_ARG_INFO(0, ticks)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_memtrigger_register, 0, 0, 2)
		ZEND_ARG_INFO(0, bytes)
		ZEND_ARG_INFO(0, callable)
ZEND_END_ARG_INFO()


/* }}} */

const zend_function_entry memtrigger_functions[] = {
	PHP_FE(memtrigger_init, arginfo_memtrigger_init)
	PHP_FE(memtrigger_register, arginfo_memtrigger_register)

	PHP_FE_END
};

zend_module_entry memtrigger_module_entry = {
	STANDARD_MODULE_HEADER,
	"memtrigger",
	memtrigger_functions,
	PHP_MINIT(memtrigger),
	PHP_MSHUTDOWN(memtrigger),
	PHP_RINIT(memtrigger),
	PHP_RSHUTDOWN(memtrigger),
	PHP_MINFO(memtrigger),
	PHP_MEMTRIGGER_VERSION,
	PHP_MODULE_GLOBALS(memtrigger),
	PHP_GINIT(memtrigger),
	NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};

#ifdef COMPILE_DL_MEMTRIGGER
ZEND_GET_MODULE(memtrigger)
#endif



void php_register_memtrigger_constants(INIT_FUNC_ARGS)
{
	/* Signal Constants */
//	REGISTER_LONG_CONSTANT("SIG_IGN",  (zend_long) SIG_IGN, CONST_CS | CONST_PERSISTENT);
}

static void php_memtrigger_register_errno_constants(INIT_FUNC_ARGS)
{
//#ifdef EINTR
//	REGISTER_MSL_ERRNO_CONSTANT(EINTR);
//#endif
}

static PHP_GINIT_FUNCTION(memtrigger)
{
	memset(memtrigger_globals, 0, sizeof(*memtrigger_globals));
}

PHP_RINIT_FUNCTION(memtrigger)
{
	MEMTRIGGER_G(user_tick_functions) = NULL;
	MEMTRIGGER_G(ticks_between_mem_check) = 100;
	MEMTRIGGER_G(ticks_till_next_mem_check) = MEMTRIGGER_G(ticks_between_mem_check);

	return SUCCESS;
}

PHP_MINIT_FUNCTION(memtrigger)
{
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(memtrigger)
{
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(memtrigger)
{
	if (MEMTRIGGER_G(user_tick_functions)) {
		zend_llist_destroy(MEMTRIGGER_G(user_tick_functions));
		efree(MEMTRIGGER_G(user_tick_functions));
		MEMTRIGGER_G(user_tick_functions) = NULL;
	}

	return SUCCESS;
}

PHP_MINFO_FUNCTION(memtrigger)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "memtrigger support", "enabled");
	php_info_print_table_end();
}


/* {{{ proto bool msl_init(int $ticks)
    */
PHP_FUNCTION(memtrigger_init)
{
	long ticks_between_mem_check = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &ticks_between_mem_check) == FAILURE) {
		printf("Failed to parse\n");
		RETURN_FALSE;
	}

	MEMTRIGGER_G(ticks_between_mem_check) = ticks_between_mem_check;
	
	printf("Setting ticks_between_mem_check to %d\n", ticks_between_mem_check);

}
/* }}} */

/* {{{ proto bool register_tick_function(callable $callback)
   Registers a tick callback function */
PHP_FUNCTION(memtrigger_register)
{
	memtrigger_tick_function_entry tick_fe;
	zval *user_callback;
	long triggerBytes = 0;
	long resetBytes = 0;

	/* Parse parameters given to function */
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lz|l", &triggerBytes, &user_callback, &resetBytes) == FAILURE) {
		printf("Failed to parse\n");
		RETURN_FALSE;
	}

	// Check whether the callback is valid now, rather than failing later
	if (!user_callback || !zend_is_callable(user_callback, 0, NULL TSRMLS_CC)) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC,
			"Invalid argument, callback must be a callable.");
		RETURN_FALSE;
	}

	Z_ADDREF_P(user_callback);

	tick_fe.callable = user_callback;
	tick_fe.triggerBytes = triggerBytes;
	tick_fe.resetBytes = resetBytes;
	tick_fe.calling = 0;
	tick_fe.triggered = 0;

	if (!MEMTRIGGER_G(user_tick_functions)) {
		MEMTRIGGER_G(user_tick_functions) = (zend_llist *) emalloc(sizeof(zend_llist));
		zend_llist_init(MEMTRIGGER_G(user_tick_functions),
						sizeof(memtrigger_tick_function_entry),
						(llist_dtor_func_t) memtrigger_tick_function_dtor, 0);
		php_add_tick_function(run_memtrigger_tick_functions);
	}

	zend_llist_add_element(MEMTRIGGER_G(user_tick_functions), &tick_fe);

	RETURN_TRUE;
}
/* }}} */


static void run_memtrigger_tick_functions(int tick_count) /* {{{ */
{
	TSRMLS_FETCH();
	zend_llist *l = MEMTRIGGER_G(user_tick_functions);
	zend_llist_element *element;
	int real_usage = 1;
	long memory_usage;
	int triggered = 0;

	MEMTRIGGER_G(ticks_till_next_mem_check) -= tick_count;

	if (MEMTRIGGER_G(ticks_till_next_mem_check) > 0) {
		return;
	}

	memory_usage = zend_memory_usage(real_usage TSRMLS_CC);

	for (element=l->head; element; element=element->next) {
		triggered += memtrigger_tick_function_call((memtrigger_tick_function_entry *)element->data, memory_usage TSRMLS_CC);
	}
	
	if (triggered) {
		MEMTRIGGER_G(ticks_between_mem_check) = 50;
	}

	MEMTRIGGER_G(ticks_till_next_mem_check) = MEMTRIGGER_G(ticks_between_mem_check);
}
/* }}} */


static int memtrigger_tick_function_call(memtrigger_tick_function_entry *tick_fe, long memory_usage TSRMLS_DC) /* {{{ */
{
	zval retval;
	int error;
	zend_fcall_info fci;
	zend_fcall_info_cache fci_cache;

#ifdef ZEND_ENGINE_3
	zval zargs[2];
	zval retval;
#else
	zval **zargs[2];
	zval *retval_ptr;
#endif

	if (memory_usage < tick_fe->triggerBytes) {
		if (tick_fe->triggered) {
			if (memory_usage < tick_fe->resetBytes) {
				tick_fe->triggered = 0;
			}
		}
		return 0;
	}

	if (tick_fe->triggered) {
		return 0;
	}

	//tick_fe->triggered = 1;

	if (!tick_fe->calling) {
		tick_fe->calling = 1;

		fci_cache = empty_fcall_info_cache;
		fci.size = sizeof(fci);
		fci.function_table = EG(function_table);
	#ifdef ZEND_ENGINE_3
		fci.object = NULL;
		ZVAL_COPY_VALUE(&fci.function_name, &tick_fe->callable);
		fci.retval = &retval;
	#else
		retval_ptr = NULL;
		fci.object_ptr = NULL;
		fci.function_name = tick_fe->callable;
		fci.retval_ptr_ptr = &retval_ptr;
	#endif
		fci.param_count = 0;
		fci.params = NULL;
		fci.no_separation = 0;
		fci.symbol_table = NULL;

		error = zend_call_function(&fci, &fci_cache TSRMLS_CC);

		if (error == FAILURE) {
			// TODO - throw exception
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "An error occurred while invoking the callback");
		}

		tick_fe->calling = 0;
	}

	return 1;
}
/* }}} */



static void memtrigger_tick_function_dtor(memtrigger_tick_function_entry *tick_function_entry) /* {{{ */
{
	//remove reference to the callable zval
}
/* }}} */



/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
