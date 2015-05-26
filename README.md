# MemTrigger

A PHP extension to trigger events based on memory usage.

## Example

```php

$logAndGC = function () {
	echo "** Memory warning limit exceeded exceeded, calling GC when mem usage at ".number_format(memory_get_usage(false))."\n";
	gc_collect_cycles();
	echo "** Mem used now : ".number_format(memory_get_usage(false))."\n";
};

$MB = 1024 * 1024;
$logAndGCTrigger = new Trigger($logAndGC, 8 * $MB, Trigger::ACTION_LEAVE_ACTIVE);

trigger_init(5);
trigger_register($logAndGCTrigger);

```

The class methods and constants are documented in ./stubs.php


## How to compile

phpize
./configure 
make install


By default the implementation hooks into the memory allocations for PHP 7. For PHP 5.6 it hook into the opcode execution.



To specify that the tracking 


## How it works and limitations

-dtrigger.mode=mem

php -dextension=./modules/trigger.so -dtrigger.mode=opcode ./test.php 


### Memory allocation hook




### Opcode hook

In this mode, the extension replaces zend_execute_ex with a new function that both monitors the memory usage and calls the triggers when necessary, as well as calling the previous zend_execute_ex function.

Because of this it is not possible for all out of memory problems to be avoided. If either a single massive allocation uses more than the available remaining memory, or several very large allocations take quickly, this extension will not have a chance to detect and call a trigger about the large memory use.
 

## TODO

* Currently there is not enough control over how frequently the triggers are called.

* Check that it actually works on real code.

* Allow triggers be checked at different rates.