--TEST--
Test memory soft limit functionality
--SKIPIF--
<?php
	if (!extension_loaded("msl")) print "skip"; 
?>
--FILE--
<?php 

function foo() {
}

msl_register(1024, 'foo');

echo "Module is loaded.\n";


?>
--EXPECT--
Module is loaded.