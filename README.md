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
$logAndGCTrigger = new MemTrigger($logAndGC, 8 * $MB, MemTrigger::ACTION_LEAVE_ACTIVE);

memtrigger_init(5);
memtrigger_register($logAndGCTrigger);

```

The class methods and constants are documented in ./stubs.php


## How to compile

phpize
./configure
make install

## How it works and limitations

The extension replaces zend_execute_ex with a new function that both monitors the memory usage and calls the triggers when necessary, as well as calling the previous zend_execute_ex function.

Because of this it is not possible for all out of memory problems to be avoided. If either a single massive allocation uses more than the available remaining memory, or several very large allocations take quickly, this extension will not have a chance to detect and call a trigger about the large memory use.

The situation this library is useful for is when a script uses up memory continually without releasing it. 



## TODO

* Currently there is not enough control over how frequently the triggers are called.

* Check that it actually works on real code.

* Allow triggers be checked at different rates.