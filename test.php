<?php

declare(ticks=100);


function logFunction()
{
	echo "Mem soft limit exceeded.\n";
}

function otherScope()
{
	echo "******************";
	echo "Next Mem soft limit exceeded, calling GC when mem usage at ".memory_get_usage(true)."\n";
	gc_collect_cycles();
	echo "Mem used now : ".memory_get_usage(true)."\n";
}

function logAndGC()
{
///	gc_collect_cycles();
	echo "******************";
	echo "Next Mem soft limit exceeded, calling GC when mem usage at ".memory_get_usage(true)."\n";
	gc_collect_cycles();
	gc_collect_cycles();
	gc_collect_cycles();
	gc_collect_cycles();
	gc_collect_cycles();
	gc_collect_cycles();
	echo "Mem used now : ".memory_get_usage(true)."\n";
}


function logAndGC2()
{
	echo "Next Mem soft limit exceeded2, calling GC when mem usage at ".memory_get_usage(true)."\n";
	gc_collect_cycles();
	echo "Mem used now : ".memory_get_usage(true)."\n";
}


function abortOperation()
{
	throw new \Exception("Too much memory being used");
}

memtrigger_init(5000);

//msl_register(1024 * 1024 * 4, 'logFunction');
memtrigger_register(1024 * 1024 * 8, 'logAndGC');
//msl_register(1024 * 1024 * 32, 'logAndGC2');
//msl_register(1024 * 1024 * 64, 'abortOperation');

useMemoryUp(false);

function useMemoryUp($performGC = false) {

	$memData = '';

	if ($performGC) {
		echo "GC collection will be done - app should not crash.\n";
	}
	else {
		echo "GC collection won't be done - app should crash.\n";
	}
	$dataSizeInKB = 128;

	//Change this line if you tweak the parameters above.
	ini_set('memory_limit', "64M");
	
	for ($y=0 ; $y<$dataSizeInKB ; $y++) {
		for ($x=0 ; $x<32 ; $x++) { //1kB
			$memData .= md5(time() + (($y * 32) + $x));
		}
	}

	file_put_contents("memdata.txt", $memData);

	// This function creates a cyclic variable loop
	function useSomeMemory($x) {
		$data = [];
		$data[$x] = file_get_contents("memdata.txt");
		$data[$x + 1] = &$data;
	};

	for($x=0 ; $x<1000 ; $x++) {
		if (($x%10) == 0) {
			echo "x = $x   " . number_format(memory_get_usage(true)) . "  " . memory_get_usage(false) . "\n";
		}
//		$b = 0;
//		for ($y=0 ; $y<100 ; $y++) {
//			$a = $b + rand(0, 4);
//			$b = $a / 2;
//		}

		useSomeMemory($x);
		if ($performGC == true) {
			gc_collect_cycles();
		}
//		else {
//			echo "would have called gc_collect\n";
//		}
	}

	printf("\nused: %10d | allocated: %10d | peak: %10d\n",
		memory_get_usage(),
		memory_get_usage(true),
		memory_get_peak_usage(true)
	);
}