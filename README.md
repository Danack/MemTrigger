# MemTrigger
PHP extension to trigger events based on memory usage.

## Usage

Compile the extension 

```php


memtrigger_register(
	'logAndGC',	//The callable to call
	8 * $MB,	// The memory limit to activate the trigger 
	0,			// Whether to deactive the trigger after it's been triggered.
	8 * $MB,	// The reset limit below which the memory usage must fall for the trigger to be reset.
);


function logAndGC()
{
	echo "** Memory warning limit exceeded exceeded, calling GC when mem usage at ".number_format(memory_get_usage(false))."\n";
	gc_collect_cycles();
	echo "** Mem used now : ".number_format(memory_get_usage(false))."\n";
}

```