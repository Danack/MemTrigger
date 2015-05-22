--TEST--
Test memory soft limit functionality
--SKIPIF--
<?php
	if (!extension_loaded("memtrigger")) print "skip memtrigger not loaded"; 
?>
--FILE--
<?php 

require_once "functions.php";

$wasCalled = false;

$logAndGC = function () use (&$wasCalled) {
	gc_collect_cycles();
	$wasCalled = true;
};

$MB = 1024 * 1024;
$logAndGCTrigger = new MemTrigger($logAndGC, 8 * $MB, MemTrigger::ACTION_LEAVE_ACTIVE);

memtrigger_init(5);
memtrigger_register($logAndGCTrigger);

echo "Module is loaded.\n";

useLotsOfMemory();

if ($wasCalled == false) {
	echo "Trigger was not called.";
}



?>

--CLEAN--
<?php 
@unlink("./memdata.txt");
?>
--EXPECT--
Module is loaded.