
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

#include "memtrigger.h"
#include "php_ticks.h"

#include "zend.h"
#include "zend_compile.h"
#include "zend_execute.h"
#include "zend_exceptions.h"
#include "zend_gc.h"
#include "zend_API.h"
#include "ext/spl/spl_exceptions.h"

#include <errno.h>

ZEND_DECLARE_MODULE_GLOBALS(memtrigger)
static PHP_GINIT_FUNCTION(memtrigger);

// Defines


// Structs


// Variables

int memtrigger_execute_initialized = 0;
static zend_object_handlers memtrigger_object_handlers;
zend_class_entry *php_memtrigger_sc_entry;
zend_class_entry *php_memtrigger_exception_class_entry;

// Forward Declarations

// Data



ZEND_BEGIN_ARG_INFO_EX(memtrigger_zero_args, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(memtrigger_constructor_args, 0, 0, 1)
	ZEND_ARG_INFO(0, callback)
	ZEND_ARG_INFO(0, value)
	ZEND_ARG_INFO(0, action)
	ZEND_ARG_INFO(0, state)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(memtrigger_value_arg, 0, 0, 1)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(memtrigger_setState_args, 0, 0, 1)
	ZEND_ARG_INFO(0, state)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(memtrigger_setTriggerAction_args, 0, 0, 1)
	ZEND_ARG_INFO(0, action)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_memtrigger_void, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_memtrigger_init, 0, 0, 1)
	ZEND_ARG_INFO(0, ticks)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_memtrigger_register, 0, 0, 1)
	ZEND_ARG_OBJ_INFO(0, MemTrigger, MemTrigger, 0)
ZEND_END_ARG_INFO()


// Data
const zend_function_entry memtrigger_functions[] = {
	PHP_FE(memtrigger_init, arginfo_memtrigger_init)
	PHP_FE(memtrigger_register, arginfo_memtrigger_register)

	PHP_FE_END
};

static zend_function_entry php_memtrigger_class_methods[] =
{
	PHP_ME(memtrigger, __construct, memtrigger_constructor_args, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(memtrigger, setValue, memtrigger_value_arg, ZEND_ACC_PUBLIC)
	PHP_ME(memtrigger, getValue, memtrigger_zero_args, ZEND_ACC_PUBLIC)
	PHP_ME(memtrigger, setState, memtrigger_setState_args, ZEND_ACC_PUBLIC)
	PHP_ME(memtrigger, getState, memtrigger_zero_args, ZEND_ACC_PUBLIC)
	PHP_ME(memtrigger, setAction, memtrigger_setTriggerAction_args, ZEND_ACC_PUBLIC)
	PHP_ME(memtrigger, getAction, memtrigger_zero_args, ZEND_ACC_PUBLIC)

	{ NULL, NULL, NULL }
};

static void php_memtrigger_object_free_storage(MEMTRIGGER_ZEND_OBJECT *object TSRMLS_DC)
{
	php_memtrigger_object *intern = php_memtrigger_fetch_object(object);

	if (!intern) {
		return;
	}

#ifndef ZEND_ENGINE_3
	zend_object_std_dtor(&intern->zo TSRMLS_CC);
	efree(intern);
#endif
}


#ifdef ZEND_ENGINE_3
static zend_object *php_memtrigger_object_new_ex(zend_class_entry *class_type, php_memtrigger_object **ptr TSRMLS_DC)
#else
static zend_object_value php_memtrigger_object_new_ex(zend_class_entry *class_type, php_memtrigger_object **ptr TSRMLS_DC)
#endif
{
	php_memtrigger_object *intern;

	/* Allocate memory for it */
#ifdef ZEND_ENGINE_3
	intern = ecalloc(1,
		sizeof(php_memtrigger_object) +
		sizeof(zval) * (class_type->default_properties_count - 1));
#else
	zend_object_value retval;
	intern = (php_memtrigger_object *) emalloc(sizeof(php_memtrigger_object));
		memset(&intern->zo, 0, sizeof(zend_object));
#endif

	if (ptr) {
		*ptr = intern;
	}

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	object_properties_init(&intern->zo, class_type);

#ifdef ZEND_ENGINE_3
	intern->zo.handlers = &memtrigger_object_handlers;

	return &intern->zo;
#else
	retval.handle = zend_objects_store_put(
		intern, 
		NULL,
		(zend_objects_free_object_storage_t) php_memtrigger_object_free_storage,
		NULL TSRMLS_CC
	);
	retval.handlers = (zend_object_handlers *) &memtrigger_object_handlers;

	return retval;
#endif
}

#ifdef ZEND_ENGINE_3
static zend_object * php_memtrigger_object_new(zend_class_entry *class_type TSRMLS_DC) {
	return php_memtrigger_object_new_ex(class_type, NULL TSRMLS_CC);
}
#else
static zend_object_value php_memtrigger_object_new(zend_class_entry *class_type TSRMLS_DC) {
	return php_memtrigger_object_new_ex(class_type, NULL TSRMLS_CC);
}
#endif


//Forward declare functions and function pointers
#if PHP_VERSION_ID >= 70000
ZEND_API void memtrigger_execute_ex(zend_execute_data *execute_data);
ZEND_API void (*memtrigger_old_execute)(zend_execute_data *execute_data);
#elif PHP_VERSION_ID >= 50500
void memtrigger_execute_ex(zend_execute_data *execute_data TSRMLS_DC);
void (*memtrigger_old_execute)(zend_execute_data *execute_data TSRMLS_DC);
#else
void memtrigger_execute(zend_op_array *op_array TSRMLS_DC);
void (*memtrigger_old_execute)(zend_op_array *op_array TSRMLS_DC);
#endif

static int memtrigger_tick_function_call(php_memtrigger_object *memtrigger, long memory_usage TSRMLS_DC);

/* some prototypes for local functions */


/* {{{ PHP_INI
 */
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("memtrigger.enabled", "1", PHP_INI_SYSTEM, OnUpdateBool, enabled, zend_memtrigger_globals, memtrigger_globals)
PHP_INI_END()
/* }}} */


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
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE();
#endif
ZEND_GET_MODULE(memtrigger)
#endif


static PHP_GINIT_FUNCTION(memtrigger)
{
	memset(memtrigger_globals, 0, sizeof(*memtrigger_globals));
}

PHP_RINIT_FUNCTION(memtrigger)
{
	if (!MEMTRIGGER_G(enabled)) {
		return SUCCESS;
	}

	MEMTRIGGER_G(user_triggers) = NULL;
	MEMTRIGGER_G(ticks_between_mem_check) = 100;
	MEMTRIGGER_G(ticks_till_next_mem_check) = MEMTRIGGER_G(ticks_between_mem_check);

	if (!MEMTRIGGER_G(initialized)) {
#if PHP_VERSION_ID >= 50500
		memtrigger_old_execute = zend_execute_ex;
		zend_execute_ex = memtrigger_execute_ex;
#else
		memtrigger_old_execute = zend_execute;
		zend_execute = memtrigger_execute_ex;
#endif
//		// TODO - do we care about this?
//		if (zend_execute_internal) {
//			memtrigger_old_execute_internal = zend_execute_internal;
//			zend_execute_internal = memtrigger_execute_internal;
//		}
		MEMTRIGGER_G(initialized) = 1;
	}

	ALLOC_HASHTABLE(MEMTRIGGER_G(user_triggers));
	ZEND_INIT_SYMTABLE_EX(MEMTRIGGER_G(user_triggers), 1, 0);

	return SUCCESS;
}

static void php_memtrigger_init_globals(zend_memtrigger_globals *memtrigger_globals)
{
	memtrigger_globals->initialized = 0;
	memtrigger_globals->enabled = 0;
	memtrigger_globals->user_triggers = NULL;
	memtrigger_globals->ticks_between_mem_check = 0;
	memtrigger_globals->ticks_till_next_mem_check = 0;
}


PHP_MINIT_FUNCTION(memtrigger)
{
	zend_class_entry ce;

	ZEND_INIT_MODULE_GLOBALS(memtrigger, php_memtrigger_init_globals, NULL);
	memcpy(&memtrigger_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

	INIT_CLASS_ENTRY(ce, PHP_MEMTRIGGER_EXCEPTION_SC_NAME, NULL);
	#ifdef ZEND_ENGINE_3
		php_memtrigger_exception_class_entry = zend_register_internal_class_ex(&ce, zend_exception_get_default(TSRMLS_C) TSRMLS_CC);
	#else
		php_memtrigger_exception_class_entry = zend_register_internal_class_ex(&ce, zend_exception_get_default(TSRMLS_C), NULL TSRMLS_CC);
	#endif


	INIT_CLASS_ENTRY(ce, PHP_MEMTRIGGER_SC_NAME, php_memtrigger_class_methods);
	ce.create_object = php_memtrigger_object_new;
	//imagick_object_handlers.clone_obj = php_imagick_clone_imagick_object;
	//memtrigger_object_handlers.read_property = php_memtrigger_read_property;
	//imagick_object_handlers.count_elements = php_imagick_count_elements;
#ifdef ZEND_ENGINE_3
	memtrigger_object_handlers.offset = XtOffsetOf(php_memtrigger_object, zo);
	memtrigger_object_handlers.free_obj = php_memtrigger_object_free_storage;
#endif

	php_memtrigger_sc_entry = zend_register_internal_class(&ce TSRMLS_CC);
	php_memtrigger_initialize_constants(TSRMLS_C);
	REGISTER_INI_ENTRIES();

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(memtrigger)
{
	UNREGISTER_INI_ENTRIES();

	if (memtrigger_execute_initialized) {
		#if PHP_VERSION_ID >= 50500
			zend_execute_ex = memtrigger_old_execute;
		#else
			zend_execute = memtrigger_old_execute;
		#endif
	}

	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(memtrigger)
{
	if (MEMTRIGGER_G(user_triggers)) {
#ifndef ZEND_ENGINE_3
		//efree(MEMTRIGGER_G(user_triggers));
#endif
		MEMTRIGGER_G(user_triggers) = NULL;
	}

	return SUCCESS;
}

PHP_MINFO_FUNCTION(memtrigger)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "memtrigger support", "enabled");
	php_info_print_table_end();
}


/* {{{ proto bool memtrigger_init(int $ticks)
    */
PHP_FUNCTION(memtrigger_init)
{
	long ticks_between_mem_check = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &ticks_between_mem_check) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid parameters for memtrigger_init");
		RETURN_FALSE;
	}

	MEMTRIGGER_G(ticks_between_mem_check) = ticks_between_mem_check;
}
/* }}} */

/* {{{ proto bool register_tick_function(callable $callback)
   Registers a tick callback function */
PHP_FUNCTION(memtrigger_register)
{
	php_memtrigger_object *memtrigger;
	zval *objvar;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &objvar, php_memtrigger_sc_entry) == FAILURE) {
		return;
	}

	//memtrigger = Z_MEMTRIGGER_P(objvar);
	Z_ADDREF_P(objvar);

#ifdef ZEND_ENGINE_3
	zend_hash_next_index_insert(MEMTRIGGER_G(user_triggers), objvar);
#else
	zend_hash_next_index_insert(MEMTRIGGER_G(user_triggers), &objvar, sizeof(zval *), NULL);
#endif

	RETURN_TRUE;
}
/* }}} */

#ifdef ZEND_ENGINE_3


static void run_memtrigger_tick_functions(int opcodes) /* {{{ */
{
	TSRMLS_FETCH();

	zend_llist_element *element;
	long memory_usage;
	int triggers_run = 0;
	zval *pzval;
	int i;
	php_memtrigger_object *trigger;

	HashTable *triggers = MEMTRIGGER_G(user_triggers);
	//struct timeval tp = {0};
	//gettimeofday(&tp, NULL)

//	if (opcodes) {
//		printf("opcodes = %d\n", opcodes);
//	}

	MEMTRIGGER_G(ticks_till_next_mem_check) -= 1;

	if (MEMTRIGGER_G(ticks_till_next_mem_check) > 0) {
		return;
	}

	memory_usage = zend_memory_usage(0 TSRMLS_CC);

	ZEND_HASH_FOREACH_VAL(triggers, pzval) {
		trigger = Z_MEMTRIGGER_P(pzval);
		triggers_run += memtrigger_tick_function_call(trigger, memory_usage TSRMLS_CC);
	} ZEND_HASH_FOREACH_END();

	if (triggers_run) {
		if (MEMTRIGGER_G(ticks_between_mem_check) > 50) {
			// Do something clever to make checks more frequent when something has been triggered.
			// MEMTRIGGER_G(ticks_between_mem_check) = 50;
		}
	}

	MEMTRIGGER_G(ticks_till_next_mem_check) = MEMTRIGGER_G(ticks_between_mem_check);
}


#else 

static void run_memtrigger_tick_functions(int opcodes) /* {{{ */
{
	TSRMLS_FETCH();

	zend_llist_element *element;
	long memory_usage;
	int triggers_run = 0;
	zval **ppzval;
	int i;
	php_memtrigger_object *trigger;

	HashTable *triggers = MEMTRIGGER_G(user_triggers);
	//struct timeval tp = {0};
	//gettimeofday(&tp, NULL)
	
//	if (opcodes) {
//		printf("opcodes = %d\n", opcodes);
//	}

	MEMTRIGGER_G(ticks_till_next_mem_check) -= 1;

	if (MEMTRIGGER_G(ticks_till_next_mem_check) > 0) {
		return;
	}

	memory_usage = zend_memory_usage(0 TSRMLS_CC);

	for (i = 0, zend_hash_internal_pointer_reset(triggers);
			zend_hash_get_current_data(triggers, (void **) &ppzval) == SUCCESS;
			zend_hash_move_forward(triggers), i++){
		trigger = Z_MEMTRIGGER_P(*ppzval);
		triggers_run += memtrigger_tick_function_call(trigger, memory_usage TSRMLS_CC);
	}

	if (triggers_run) {
		if (MEMTRIGGER_G(ticks_between_mem_check) > 50) {
			// Do something clever to make checks more frequent when something has been triggered.
			// MEMTRIGGER_G(ticks_between_mem_check) = 50;
		}
	}

	MEMTRIGGER_G(ticks_till_next_mem_check) = MEMTRIGGER_G(ticks_between_mem_check);
}
#endif


/* }}} */


static int memtrigger_tick_function_call(php_memtrigger_object *memtrigger, long memory_usage TSRMLS_DC) /* {{{ */
{
	//zval retval;
	int error;
	zend_fcall_info fci;
	zend_fcall_info_cache fci_cache;

#ifdef ZEND_ENGINE_3
	zval retval;
#else
	zval *retval_ptr;
#endif

	if (memtrigger->state == MEMTRIGGER_STATE_DISABLED) {
		return 0;
	}

	switch (memtrigger->type) {
		case (MEMTRIGGER_TYPE_ABOVE): {
			//Only trigger when memory is above trigger value
			if (memory_usage < memtrigger->value) {
				return 0;
			}
			break; 
		}

		case(MEMTRIGGER_TYPE_BELOW): {
			//Only trigger when memory is below trigger value
			if (memory_usage > memtrigger->value) {
				return 0;
			}
			break;
		}

		default:
			//TODO need to throw an exception...
			return;
	}

	// trigger has been triggered.
	if (memtrigger->action == MEMTRIGGER_ACTION_DISABLE) {
		memtrigger->state = MEMTRIGGER_STATE_DISABLED;
	}

	if (!memtrigger->calling) {
		memtrigger->calling = 1;

		fci_cache = empty_fcall_info_cache;
		fci.size = sizeof(fci);
		fci.function_table = EG(function_table);
	#ifdef ZEND_ENGINE_3
		fci.object = NULL;
		ZVAL_COPY_VALUE(&fci.function_name, &memtrigger->callable);
		fci.retval = &retval;
	#else
		retval_ptr = NULL;
		fci.object_ptr = NULL;
		fci.function_name = memtrigger->callable;
		fci.retval_ptr_ptr = &retval_ptr;
	#endif
		fci.param_count = 0;
		fci.params = NULL;
		fci.no_separation = 0;
		fci.symbol_table = NULL;

		error = zend_call_function(&fci, &fci_cache TSRMLS_CC);

		if (error == FAILURE) {
			// TODO - throw an appropriate exception
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "An error occurred while invoking the callback");
		}

		memtrigger->calling = 0;
	}

	return 1;
}
/* }}} */

//#define MEMTRIGGER_STABLE

#ifdef MEMTRIGGER_STABLE
#if PHP_VERSION_ID >= 50500
void memtrigger_execute(zend_execute_data *execute_data TSRMLS_DC) /* {{{ */
{
#else
void memtrigger_execute(zend_op_array *op_array TSRMLS_DC) /* {{{ */
{
#endif
	run_memtrigger_tick_functions();

#if PHP_VERSION_ID >= 50500
	memtrigger_old_execute(execute_data TSRMLS_CC);
#else
	memtrigger_old_execute(op_array TSRMLS_CC);
#endif

}

#else


#ifdef ZEND_ENGINE_3

#ifdef ZEND_VM_FP_GLOBAL_REG
# define ZEND_OPCODE_HANDLER_RET int
# define ZEND_OPCODE_HANDLER_ARGS void
# define ZEND_OPCODE_HANDLER_ARGS_PASSTHRU
# define ZEND_OPCODE_HANDLER_ARGS_DC
# define ZEND_OPCODE_HANDLER_ARGS_PASSTHRU_CC
#else
# define ZEND_OPCODE_HANDLER_RET int
# define ZEND_OPCODE_HANDLER_ARGS zend_execute_data *execute_data
# define ZEND_OPCODE_HANDLER_ARGS_PASSTHRU execute_data
# define ZEND_OPCODE_HANDLER_ARGS_DC , ZEND_OPCODE_HANDLER_ARGS
# define ZEND_OPCODE_HANDLER_ARGS_PASSTHRU_CC , ZEND_OPCODE_HANDLER_ARGS_PASSTHRU
#endif



#undef OPLINE
#undef DCL_OPLINE
#undef USE_OPLINE
#undef LOAD_OPLINE
#undef SAVE_OPLINE
#define DCL_OPLINE
#ifdef ZEND_VM_IP_GLOBAL_REG
# define OPLINE opline
# define USE_OPLINE
# define LOAD_OPLINE() opline = EX(opline)
# define SAVE_OPLINE() EX(opline) = opline
#else
# define OPLINE EX(opline)
# define USE_OPLINE const zend_op *opline = EX(opline);
# define LOAD_OPLINE()
# define SAVE_OPLINE()
#endif


typedef ZEND_OPCODE_HANDLER_RET (ZEND_FASTCALL *opcode_handler_t) (ZEND_OPCODE_HANDLER_ARGS);


ZEND_API void memtrigger_execute_ex(zend_execute_data *ex)
{
	int oplines = 0;
	//DCL_OPLINE

#ifdef ZEND_VM_IP_GLOBAL_REG
	const zend_op *orig_opline = opline;
#endif
#ifdef ZEND_VM_FP_GLOBAL_REG
	zend_execute_data *orig_execute_data = execute_data;
	execute_data = ex;
#else
	zend_execute_data *execute_data = ex;
#endif


//	LOAD_OPLINE();

	while (1) {
		int ret;
		oplines++;
		if (UNEXPECTED((ret = ((opcode_handler_t)OPLINE->handler)(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU)) != 0)) {
#ifdef ZEND_VM_FP_GLOBAL_REG
			execute_data = orig_execute_data;
# ifdef ZEND_VM_IP_GLOBAL_REG
			opline = orig_opline;
# endif
			goto end;
#else
			if (EXPECTED(ret > 0)) {
				execute_data = EG(current_execute_data);
			} else {
# ifdef ZEND_VM_IP_GLOBAL_REG
				opline = orig_opline;
# endif
				goto end;
			}
#endif
		}

	}
	zend_error_noreturn(E_ERROR, "Arrived at end of main loop which shouldn't happen");


end:
	run_memtrigger_tick_functions(oplines);
}

#else

ZEND_API void memtrigger_execute_ex(zend_execute_data *execute_data TSRMLS_DC)
{
	zend_bool original_in_execution;
	int op_codes = 1;

	original_in_execution = EG(in_execution);
	EG(in_execution) = 1;


	if (0) {
zend_vm_enter:


#if PHP_VERSION_ID >= 50500
	execute_data =  zend_create_execute_data_from_op_array(EG(active_op_array), 1 TSRMLS_CC);
#else
	#error not supported
#endif 
	}

	while (1) {
    	int ret;
#ifdef ZEND_WIN32
		if (EG(timed_out)) {
			zend_timeout(0);
		}
#endif

		op_codes++;
		zend_op *opline = execute_data->opline;
		if ((ret = opline->handler(execute_data TSRMLS_CC)) > 0) {
			switch (ret) {
				case 1:
					EG(in_execution) = original_in_execution;
					goto end;// op_codes;
				case 2:
					goto zend_vm_enter;
					break;
				case 3:
					execute_data = EG(current_execute_data);
					break;
				default:
					break;
			}
		}
	}
	zend_error_noreturn(E_ERROR, "Arrived at end of main loop which shouldn't happen");

end:
	run_memtrigger_tick_functions(op_codes);
}
#endif //ZEND_ENGINE_3

#endif




#if PHP_VERSION_ID >= 50500
void memtrigger_execute_internal(zend_execute_data *execute_data_ptr, struct _zend_fcall_info *fci, int return_value_used TSRMLS_DC)
#else
void memtrigger_execute_internal(zend_execute_data *current_execute_data, int return_value_used TSRMLS_DC) /* {{{ */
#endif
{

	#if PHP_VERSION_ID >= 50500
	memtrigger_old_execute_internal(execute_data_ptr, fci, return_value_used TSRMLS_CC);
	#else
	memtrigger_old_execute_internal(current_execute_data, return_value_used TSRMLS_CC);
	#endif
}
/* }}} */



/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
