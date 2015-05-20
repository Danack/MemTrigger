--TEST--
Test memory soft limit functionality
--SKIPIF--
<?php
	if (!extension_loaded("memtrigger")) print "skip memtrigger not loaded"; 
?>
--FILE--
<?php 

function foo() {
}

memtrigger_register('foo', 1024);

echo "Module is loaded.\n";

?>
--EXPECT--
Module is loaded.