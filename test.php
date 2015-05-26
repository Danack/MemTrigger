<?php


$logAndGC = function () {
	echo "** Memory warning limit exceeded exceeded, calling GC when mem usage at ".number_format(memory_get_usage(false))."\n";
	gc_collect_cycles();
	echo "** Mem used now : ".number_format(memory_get_usage(false))."\n";
};

$MB = 1024 * 1024;
$logAndGCTrigger = new Trigger($logAndGC, 8 * $MB, Trigger::ACTION_LEAVE_ACTIVE);


//$logTrigger = new MemTrigger('logFunction', 4 * $MB, MemTrigger::ACTION_DISABLE);
//$abortTrigger = new MemTrigger('throwException', 32 * $MB, MemTrigger::ACTION_DISABLE);


\trigger\init(5);
\trigger\register($logAndGCTrigger);


try {
	useLotsOfMemory();
}
catch (\Exception $e) {
	echo "Exception caught: ".$e->getMessage();
}

printf("\nused: %10d | allocated: %10d | peak: %10d\n",
	memory_get_usage(),
	memory_get_usage(true),
	memory_get_peak_usage(true)
);

exit(0);


function logFunction()
{
	echo "Memory info limit exceeded.\n";
}




function throwException()
{
	throw new \Exception("Memory emergency limit exceeded.");
	///return MEMTRIGGER_THROW;
}


// This function creates a cyclic variable loop
function useSomeMemory($x)
{
	$data = [];
	$data[$x] = file_get_contents("memdata.txt");
	$data[$x + 1] = &$data;
}

function bar($x)
{
	//echo "adsd".$x;
}

function useLotsOfMemory()
{
	global $ticks;
	
	$memData = '';
	$dataSizeInKB = 64;

	//Change this line if you tweak the parameters.
	ini_set('memory_limit', "64M");

	for ($y = 0; $y < $dataSizeInKB; $y++) {
		for ($x = 0; $x < 32; $x++) { //1kB
			$memData .= md5(time() + (($y * 32) + $x));
		}
	}

	file_put_contents("memdata.txt", $memData);

	for ($x = 0; $x < 2000; $x++) {

		$b = 0;
		for ($y = 0; $y < 100; $y++) {
			$a = $b + rand(0, 4);
			$b = $a / 2;
		}
		if (rand(0, 2)) {
			$b /= 2;
			$b += 0.5;
			bar($b);
		}

		if (($x % 10) == 0) {
			echo "x: $x ticks: $ticks mem: ".number_format(memory_get_usage(false))."\n";
		}

		useSomeMemory($x);
	}


}