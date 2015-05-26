
/* $Id$ */

#define TRIGGER_DEBUG 0


#if TRIGGER_DEBUG
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
#include "php_trigger.h"

#include "trigger.h"
#include "php_ticks.h"

#include "zend.h"
#include "zend_alloc.h"
#include "zend_compile.h"
#include "zend_execute.h"
#include "zend_exceptions.h"
#include "zend_gc.h"
#include "zend_API.h"
#include "ext/spl/spl_exceptions.h"

#include <errno.h>

ZEND_DECLARE_MODULE_GLOBALS(trigger)
static PHP_GINIT_FUNCTION(trigger);

// Defines

#define TRIGGER_IMPLEMENTATION_MEM 1
#define TRIGGER_IMPLEMENTATION_OPCODE 2

#define TRIGGER_IMPLEMENTATION_MEM_NAME "Memory"
#define TRIGGER_IMPLEMENTATION_OPCODE_NAME "Opcode"


// Structs


// Variables

int trigger_execute_initialized = 0;
int trigger_implementation = 0;
char *trigger_implementation_name = "Unknown";

static zend_object_handlers trigger_object_handlers;
zend_class_entry *php_trigger_sc_entry;
zend_class_entry *php_trigger_exception_class_entry;


// Forward Declarations

// Data

ZEND_BEGIN_ARG_INFO_EX(trigger_zero_args, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(trigger_constructor_args, 0, 0, 1)
	ZEND_ARG_INFO(0, callback)
	ZEND_ARG_INFO(0, value)
	ZEND_ARG_INFO(0, action)
	ZEND_ARG_INFO(0, state)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(trigger_value_arg, 0, 0, 1)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(trigger_setState_args, 0, 0, 1)
	ZEND_ARG_INFO(0, state)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(trigger_setTriggerAction_args, 0, 0, 1)
	ZEND_ARG_INFO(0, action)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_trigger_void, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_trigger_init, 0, 0, 1)
	ZEND_ARG_INFO(0, ticks)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_trigger_register, 0, 0, 1)
	ZEND_ARG_OBJ_INFO(0, Trigger, Trigger, 0)
ZEND_END_ARG_INFO()

PHP_FUNCTION(hello)
{
	php_printf("Hello world!");
}


// Data
const zend_function_entry trigger_functions[] = {
	ZEND_NS_FALIAS("trigger", init, trigger_init,arginfo_trigger_init)
	ZEND_NS_FALIAS("trigger", register, trigger_register, arginfo_trigger_register)
	PHP_FE_END
};





static zend_function_entry php_trigger_class_methods[] =
{
	PHP_ME(trigger, __construct, trigger_constructor_args, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(trigger, setValue, trigger_value_arg, ZEND_ACC_PUBLIC)
	PHP_ME(trigger, getValue, trigger_zero_args, ZEND_ACC_PUBLIC)
	PHP_ME(trigger, setState, trigger_setState_args, ZEND_ACC_PUBLIC)
	PHP_ME(trigger, getState, trigger_zero_args, ZEND_ACC_PUBLIC)
	PHP_ME(trigger, setAction, trigger_setTriggerAction_args, ZEND_ACC_PUBLIC)
	PHP_ME(trigger, getAction, trigger_zero_args, ZEND_ACC_PUBLIC)
	{ NULL, NULL, NULL }
};




static void php_trigger_object_free_storage(TRIGGER_ZEND_OBJECT *object TSRMLS_DC)
{
	php_trigger_object *intern = php_trigger_fetch_object(object);

	if (!intern) {
		return;
	}

#ifndef ZEND_ENGINE_3
	zend_object_std_dtor(&intern->zo TSRMLS_CC);
	efree(intern);
#endif
}


#ifdef ZEND_ENGINE_3
static zend_object *php_trigger_object_new_ex(zend_class_entry *class_type, php_trigger_object **ptr TSRMLS_DC)
#else
static zend_object_value php_trigger_object_new_ex(zend_class_entry *class_type, php_trigger_object **ptr TSRMLS_DC)
#endif
{
	php_trigger_object *intern;

	/* Allocate memory for it */
#ifdef ZEND_ENGINE_3
	intern = ecalloc(1,
		sizeof(php_trigger_object) +
		sizeof(zval) * (class_type->default_properties_count - 1));
#else
	zend_object_value retval;
	intern = (php_trigger_object *) emalloc(sizeof(php_trigger_object));
		memset(&intern->zo, 0, sizeof(zend_object));
#endif

	if (ptr) {
		*ptr = intern;
	}

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	object_properties_init(&intern->zo, class_type);

#ifdef ZEND_ENGINE_3
	intern->zo.handlers = &trigger_object_handlers;

	return &intern->zo;
#else
	retval.handle = zend_objects_store_put(
		intern, 
		NULL,
		(zend_objects_free_object_storage_t) php_trigger_object_free_storage,
		NULL TSRMLS_CC
	);
	retval.handlers = (zend_object_handlers *) &trigger_object_handlers;

	return retval;
#endif
}

#ifdef ZEND_ENGINE_3
static zend_object * php_trigger_object_new(zend_class_entry *class_type TSRMLS_DC) {
	return php_trigger_object_new_ex(class_type, NULL TSRMLS_CC);
}
#else
static zend_object_value php_trigger_object_new(zend_class_entry *class_type TSRMLS_DC) {
	return php_trigger_object_new_ex(class_type, NULL TSRMLS_CC);
}
#endif

//Forward declare functions and function pointers
#if PHP_VERSION_ID >= 70000
ZEND_API void trigger_execute_ex(zend_execute_data *execute_data);
ZEND_API void (*trigger_old_execute)(zend_execute_data *execute_data);
#elif PHP_VERSION_ID >= 50500
void trigger_execute_ex(zend_execute_data *execute_data TSRMLS_DC);
void (*trigger_old_execute)(zend_execute_data *execute_data TSRMLS_DC);
#else
void trigger_execute(zend_op_array *op_array TSRMLS_DC);
void (*trigger_old_execute)(zend_op_array *op_array TSRMLS_DC);
#endif


static int trigger_tick_function_call(php_trigger_object *trigger, long memory_usage TSRMLS_DC);
static void run_trigger_tick_functions(int opcodes);

/* some prototypes for local functions */


/* {{{ PHP_INI
 */
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("trigger.enabled", "1", PHP_INI_SYSTEM, OnUpdateBool, enabled, zend_trigger_globals, trigger_globals)
    STD_PHP_INI_ENTRY("trigger.mode",			NULL,	PHP_INI_ALL,		OnUpdateString,		implementation_method,		zend_trigger_globals,		trigger_globals)
PHP_INI_END()

/* }}} */


zend_module_entry trigger_module_entry = {
	STANDARD_MODULE_HEADER,
	"trigger",
	trigger_functions,
	PHP_MINIT(trigger),
	PHP_MSHUTDOWN(trigger),
	PHP_RINIT(trigger),
	PHP_RSHUTDOWN(trigger),
	PHP_MINFO(trigger),
	PHP_TRIGGER_VERSION,
	PHP_MODULE_GLOBALS(trigger),
	PHP_GINIT(trigger),
	NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};

#ifdef COMPILE_DL_TRIGGER
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE();
#endif
ZEND_GET_MODULE(trigger)
#endif


static PHP_GINIT_FUNCTION(trigger)
{
	memset(trigger_globals, 0, sizeof(*trigger_globals));
}

static inline zend_mm_heap *trigger_mm_get_heap() /* {{{ */
{
    zend_mm_heap *mm_heap;

    mm_heap = zend_mm_set_heap(NULL);
    zend_mm_set_heap(mm_heap);

    return mm_heap;
}

void* (*trigger_old_malloc)(size_t);
void  (*trigger_old_free)(void*);
void* (*trigger_old_realloc)(void*, size_t);

static void* trigger_malloc(size_t bytes) {
	zend_mm_heap *heap = trigger_mm_get_heap();

	run_trigger_tick_functions(1);

	return zend_mm_alloc(heap, bytes);
}

static void* trigger_realloc(void *pointer, size_t bytes) {
	zend_mm_heap *heap = trigger_mm_get_heap();

	run_trigger_tick_functions(1);

	return zend_mm_realloc(heap, pointer, bytes);
}

static void trigger_free(void *pointer) {
	zend_mm_heap *heap = trigger_mm_get_heap();
	if (UNEXPECTED(heap == pointer)) {
		/* TODO: heap maybe allocated by mmap(zend_mm_init) or malloc(USE_ZEND_ALLOC=0)
		 * let's prevent it from segfault for now
		 */
	} else {
		zend_mm_free(heap, pointer);
	}
}

PHP_RINIT_FUNCTION(trigger)
{
	if (!TRIGGER_G(enabled)) {
		return SUCCESS;
	}

	TRIGGER_G(ticks_between_mem_check) = 100;
	TRIGGER_G(ticks_till_next_mem_check) = TRIGGER_G(ticks_between_mem_check);
	TRIGGER_G(trigger_shutting_down);

	if (!TRIGGER_G(initialized)) {
		ALLOC_HASHTABLE(TRIGGER_G(user_triggers));
		ZEND_INIT_SYMTABLE_EX(TRIGGER_G(user_triggers), 1, 0);
		TRIGGER_G(initialized) = 1;
	}

	return SUCCESS;
}

static void php_trigger_init_globals(zend_trigger_globals *trigger_globals)
{
	trigger_globals->initialized = 0;
	trigger_globals->enabled = 0;
	trigger_globals->user_triggers = NULL;
	trigger_globals->ticks_between_mem_check = 0;
	trigger_globals->ticks_till_next_mem_check = 0;
	trigger_globals->trigger_shutting_down = 0;
	//trigger_globals->implementation_method = "opcode_mkay";
}


static void determineTriggerMode() {

#if PHP_VERSION_ID >= 70000
	trigger_implementation = TRIGGER_IMPLEMENTATION_MEM;
	trigger_implementation_name = TRIGGER_IMPLEMENTATION_MEM_NAME;
#else
	trigger_implementation = TRIGGER_IMPLEMENTATION_OPCODE;
	trigger_implementation_name = TRIGGER_IMPLEMENTATION_OPCODE_NAME; 
#endif

	if (TRIGGER_G(implementation_method) == NULL) {
		return;
	}
	if (strcasecmp(TRIGGER_G(implementation_method), "mem") == 0) {
#if PHP_VERSION_ID < 70000
		php_error_docref(NULL TSRMLS_CC,
		E_WARNING,
		"Trigger hooking into memory only supported on PHP >= 7",
		TRIGGER_G(implementation_method)
	);
#else
		trigger_implementation = TRIGGER_IMPLEMENTATION_MEM;
		trigger_implementation_name = TRIGGER_IMPLEMENTATION_MEM_NAME;
#endif
	}
	else if (strcasecmp(TRIGGER_G(implementation_method), "opcode") == 0) {
		trigger_implementation = TRIGGER_IMPLEMENTATION_OPCODE;
		trigger_implementation_name = TRIGGER_IMPLEMENTATION_OPCODE_NAME; 
	}
	else {
		php_error_docref(
			NULL TSRMLS_CC,
			E_WARNING,
			"Unknown trigger implementation %s, defaulting to %s",
			trigger_implementation_name, trigger_implementation_name);
	}
}

static void setupTriggerHook() {
	if (trigger_implementation == TRIGGER_IMPLEMENTATION_MEM) {
		zend_mm_heap *mm_heap;
		mm_heap = zend_mm_set_heap(NULL);
		zend_mm_set_heap(mm_heap);
		zend_mm_get_custom_handlers(mm_heap, &trigger_old_malloc, &trigger_old_free, &trigger_old_realloc);
		zend_mm_set_custom_handlers(mm_heap, trigger_malloc, trigger_free, trigger_realloc);
	}
	else if (trigger_implementation == TRIGGER_IMPLEMENTATION_OPCODE) {
#if PHP_VERSION_ID >= 50500
		trigger_old_execute = zend_execute_ex;
		zend_execute_ex = trigger_execute_ex;
#else
		trigger_old_execute = zend_execute;
		zend_execute = trigger_execute_ex;
#endif
	}
}

static void removeTriggerHook() {
	HashTable *user_triggers = TRIGGER_G(user_triggers);

	if (trigger_implementation == TRIGGER_IMPLEMENTATION_MEM) {
		zend_mm_heap *mm_heap;
		mm_heap = zend_mm_set_heap(NULL);
		zend_mm_set_heap(mm_heap);
		//TODO - We need to do this properly.
		*((int *) mm_heap) = 0;
	}
	else if (trigger_implementation == TRIGGER_IMPLEMENTATION_OPCODE) {
	
	}

	if (user_triggers) {
#ifndef ZEND_ENGINE_3
		//efree(TRIGGER_G(user_triggers));
#endif
	}
}

static void removeTriggerHookModule() {
	zend_mm_heap *mm_heap;

	if (trigger_execute_initialized) {
		return;
	}

	if (trigger_implementation == TRIGGER_IMPLEMENTATION_MEM) {
		//TODO - should these be restored?
//		mm_heap = zend_mm_set_heap(NULL);
//		zend_mm_set_heap(mm_heap);
//		zend_mm_set_custom_handlers(mm_heap, zend_mm_alloc, zend_mm_free, zend_mm_realloc,);

//		zend_mm_set_heap(NULL);
//		zend_mm_set_custom_handlers(mm_heap, malloc, free, realloc);
	}
	else if (trigger_implementation == TRIGGER_IMPLEMENTATION_OPCODE) {
#if PHP_VERSION_ID >= 50500
		zend_execute_ex = trigger_old_execute;
#else
		zend_execute = trigger_old_execute;
#endif
	}
}


static void setupTriggerClass() {

	zend_class_entry ce;

	memcpy(&trigger_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	INIT_CLASS_ENTRY(ce, PHP_TRIGGER_EXCEPTION_SC_NAME, NULL);
	#ifdef ZEND_ENGINE_3
		php_trigger_exception_class_entry = zend_register_internal_class_ex(&ce, zend_exception_get_default(TSRMLS_C) TSRMLS_CC);
	#else
		php_trigger_exception_class_entry = zend_register_internal_class_ex(&ce, zend_exception_get_default(TSRMLS_C), NULL TSRMLS_CC);
	#endif

	INIT_CLASS_ENTRY(ce, PHP_TRIGGER_SC_NAME, php_trigger_class_methods);
	ce.create_object = php_trigger_object_new;
#ifdef ZEND_ENGINE_3
	trigger_object_handlers.offset = XtOffsetOf(php_trigger_object, zo);
	trigger_object_handlers.free_obj = php_trigger_object_free_storage;
#endif

	php_trigger_sc_entry = zend_register_internal_class(&ce TSRMLS_CC);
	php_trigger_initialize_constants(TSRMLS_C);
}

PHP_MINIT_FUNCTION(trigger)
{
	ZEND_INIT_MODULE_GLOBALS(trigger, php_trigger_init_globals, NULL);
	setupTriggerClass();
	REGISTER_INI_ENTRIES();

	determineTriggerMode();
	setupTriggerHook();

	return SUCCESS;
}



PHP_RSHUTDOWN_FUNCTION(trigger)
{
	TRIGGER_G(trigger_shutting_down) = 1;
	//removeTriggerHook();

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(trigger)
{
	UNREGISTER_INI_ENTRIES();
	removeTriggerHook();
	removeTriggerHookModule();

	return SUCCESS;
}

PHP_MINFO_FUNCTION(trigger)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "Trigger support", "enabled");
	php_info_print_table_row(2, "Trigger implementation", trigger_implementation_name);
	php_info_print_table_end();
}


/* {{{ proto bool trigger_init(int $ticks)
    */
PHP_FUNCTION(trigger_init)
{
	long ticks_between_mem_check = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &ticks_between_mem_check) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid parameters for trigger_init");
		RETURN_FALSE;
	}

	TRIGGER_G(ticks_between_mem_check) = ticks_between_mem_check;
}
/* }}} */

/* {{{ proto bool register_tick_function(callable $callback)
   Registers a tick callback function */
PHP_FUNCTION(trigger_register)
{
	php_trigger_object *trigger;
	zval *objvar;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &objvar, php_trigger_sc_entry) == FAILURE) {
		return;
	}

	Z_ADDREF_P(objvar);

#ifdef ZEND_ENGINE_3
	zend_hash_next_index_insert(TRIGGER_G(user_triggers), objvar);
#else
	zend_hash_next_index_insert(TRIGGER_G(user_triggers), &objvar, sizeof(zval *), NULL);
#endif

	RETURN_TRUE;
}
/* }}} */

#ifdef ZEND_ENGINE_3
static void run_trigger_tick_functions(int opcodes) /* {{{ */
{
	TSRMLS_FETCH();

	zend_llist_element *element;
	long memory_usage;
	int triggers_run = 0;
	zval *pzval;
	int i;
	php_trigger_object *trigger;

	if (TRIGGER_G(trigger_shutting_down)) {
		return; 
	}

	HashTable *triggers = TRIGGER_G(user_triggers);

	if (triggers == NULL) {
		return;
	}

	TRIGGER_G(ticks_till_next_mem_check) -= 1;

	if (TRIGGER_G(ticks_till_next_mem_check) > 0) {
		return;
	}

	memory_usage = zend_memory_usage(0 TSRMLS_CC);

	ZEND_HASH_FOREACH_VAL(triggers, pzval) {
		trigger = Z_TRIGGER_P(pzval);
		triggers_run += trigger_tick_function_call(trigger, memory_usage TSRMLS_CC);
	} ZEND_HASH_FOREACH_END();

	//TODO -  Do something clever to make checks more frequent when something has been triggered.
	//if (triggers_run) {
	//	if (TRIGGER_G(ticks_between_mem_check) > 50) {
			// TRIGGER_G(ticks_between_mem_check) = 50;
	//	}
	//}

	TRIGGER_G(ticks_till_next_mem_check) = TRIGGER_G(ticks_between_mem_check);
}

#else 

static void run_trigger_tick_functions(int opcodes) /* {{{ */
{
	TSRMLS_FETCH();

	zend_llist_element *element;
	long memory_usage;
	int triggers_run = 0;
	zval **ppzval;
	int i;
	php_trigger_object *trigger;

	HashTable *triggers = TRIGGER_G(user_triggers);

	TRIGGER_G(ticks_till_next_mem_check) -= 1;

	if (TRIGGER_G(ticks_till_next_mem_check) > 0) {
		return;
	}

	memory_usage = zend_memory_usage(0 TSRMLS_CC);

	for (i = 0, zend_hash_internal_pointer_reset(triggers);
			zend_hash_get_current_data(triggers, (void **) &ppzval) == SUCCESS;
			zend_hash_move_forward(triggers), i++){
		trigger = Z_TRIGGER_P(*ppzval);
		triggers_run += trigger_tick_function_call(trigger, memory_usage TSRMLS_CC);
	}

	if (triggers_run) {
		if (TRIGGER_G(ticks_between_mem_check) > 50) {
			// Do something clever to make checks more frequent when something has been triggered.
			// TRIGGER_G(ticks_between_mem_check) = 50;
		}
	}

	TRIGGER_G(ticks_till_next_mem_check) = TRIGGER_G(ticks_between_mem_check);
}
#endif


/* }}} */
static int trigger_tick_function_call(php_trigger_object *trigger, long memory_usage TSRMLS_DC) /* {{{ */
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

	if (trigger->state == TRIGGER_STATE_DISABLED) {
		return 0;
	}

#ifdef DIRECTIONAL_TRIGGERS
	switch (trigger->type) {
		case (TRIGGER_TYPE_ABOVE): {
			//Only trigger when memory is above trigger value
			if (memory_usage < trigger->value) {
				return 0;
			}
			break; 
		}

		case(TRIGGER_TYPE_BELOW): {
			//Only trigger when memory is below trigger value
			if (memory_usage > trigger->value) {
				return 0;
			}
			break;
		}

		default:
			//TODO need to throw an exception...
			return;
	}
#else
	if (memory_usage < trigger->value) {
		return 0;
	}
#endif

	// trigger has been triggered.
	if (trigger->action == TRIGGER_ACTION_DISABLE) {
		trigger->state = TRIGGER_STATE_DISABLED;
	}

	if (!trigger->calling) {
		trigger->calling = 1;

		fci_cache = empty_fcall_info_cache;
		fci.size = sizeof(fci);
		fci.function_table = EG(function_table);
	#ifdef ZEND_ENGINE_3
		fci.object = NULL;
		ZVAL_COPY_VALUE(&fci.function_name, &trigger->callable);
		fci.retval = &retval;
	#else
		retval_ptr = NULL;
		fci.object_ptr = NULL;
		fci.function_name = trigger->callable;
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

		trigger->calling = 0;
	}

	return 1;
}
/* }}} */



#ifdef TRIGGER_STABLE
#if PHP_VERSION_ID >= 50500
void trigger_execute(zend_execute_data *execute_data TSRMLS_DC) /* {{{ */
{
#else
void trigger_execute(zend_op_array *op_array TSRMLS_DC) /* {{{ */
{
#endif
	run_trigger_tick_functions();

#if PHP_VERSION_ID >= 50500
	trigger_old_execute(execute_data TSRMLS_CC);
#else
	trigger_old_execute(op_array TSRMLS_CC);
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

ZEND_API void trigger_execute_ex(zend_execute_data *ex)
{
	int oplines = 0;

#ifdef ZEND_VM_IP_GLOBAL_REG
	const zend_op *orig_opline = opline;
#endif
#ifdef ZEND_VM_FP_GLOBAL_REG
	zend_execute_data *orig_execute_data = execute_data;
	execute_data = ex;
#else
	zend_execute_data *execute_data = ex;
#endif

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
	run_trigger_tick_functions(oplines);
}



#else

ZEND_API void trigger_execute_ex(zend_execute_data *execute_data TSRMLS_DC)
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
	run_trigger_tick_functions(op_codes);
}
#endif //ZEND_ENGINE_3

#endif


#if PHP_VERSION_ID >= 50500
void trigger_execute_internal(zend_execute_data *execute_data_ptr, struct _zend_fcall_info *fci, int return_value_used TSRMLS_DC) {
	trigger_old_execute_internal(execute_data_ptr, fci, return_value_used TSRMLS_CC);
}
#else
void trigger_execute_internal(zend_execute_data *current_execute_data, int return_value_used TSRMLS_DC) /* {{{ */
{
	trigger_old_execute_internal(current_execute_data, return_value_used TSRMLS_CC);
}
#endif
/* }}} */




/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
