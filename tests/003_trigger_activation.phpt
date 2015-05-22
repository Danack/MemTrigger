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

$MB = 1024 * 1024;

$secondFn = function () {
	throw new \Exception("SecondFn threw exception.");
};

$secondTrigger = new MemTrigger($secondFn, 32 * $MB, MemTrigger::ACTION_LEAVE_ACTIVE);
$secondTrigger->setState(MemTrigger::STATE_DISABLED);


$firstFn = function () use (&$wasCalled, $secondTrigger) {
	gc_collect_cycles();
	$wasCalled = true;
	$secondTrigger->setState(MemTrigger::STATE_ACTIVE);
};
$firstTrigger = new MemTrigger($firstFn, 8 * $MB, MemTrigger::ACTION_DISABLE);

memtrigger_init(5);
memtrigger_register($firstTrigger);
memtrigger_register($secondTrigger);


try {
	useLotsOfMemory();
}
catch(\Exception $e) {
	echo $e->getMessage()."\n";
}

if ($wasCalled == false) {
	echo "Trigger was not called.";
}


?>

--CLEAN--
<?php 
@unlink("./memdata.txt");
?>
--EXPECT--
SecondFn threw exception.