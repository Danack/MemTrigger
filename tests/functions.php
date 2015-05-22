<?php

// This function creates a cyclic variable loop
function useSomeMemory($x)
{
	$data = [];
	$data[$x] = file_get_contents("./memdata.txt");
	$data[$x + 1] = &$data;
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

	file_put_contents("./memdata.txt", $memData);

	for ($x = 0; $x < 2000; $x++) {
		$b = 0;
		for ($y = 0; $y < 100; $y++) {
			$a = $b + rand(0, 4);
			$b = $a / 2;
		}

		useSomeMemory($x);
	}
}