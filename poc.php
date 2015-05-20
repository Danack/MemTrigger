<?php

declare(ticks=5);

function logFunction()
{
    echo "Mem soft limit exceeded.\n";
}

function logAndGC()
{
    echo "Next Mem soft limit exceeded, calling GC\n";
    gc_collect_cycles();
}


function abortOperation()
{
    throw new \Exception("Too much memory being used");
}


class MemoryLimitChecker
{

    private $memoryTriggerLevels = [];

    function addLimit($triggerLevel, callable $callable)
    {
        $triggerLevel = intval($triggerLevel);
        $this->memoryTriggerLevels[$triggerLevel] = $callable;
    }


    function checkMemoryLimits()
    {
        static $lastUsage = 0;

        $currentUsage = memory_get_usage();

        foreach ($this->memoryTriggerLevels as $triggerLevel => $callable) {
            if ($currentUsage > $triggerLevel && $lastUsage <= $triggerLevel) {
                $callable();
            }
        }

        $lastUsage = $currentUsage;
    }

}




function performMemIntensiveOperation($performGC)
{
    $memData = '';

    if ($performGC) {
        echo "GC collection will be done - app should not crash.\n";
    }
    else {
        echo "GC collection won't be done - app should crash.\n";
    }
    $dataSizeInKB = 128;

//Change this line if you tweak the parameters above.
    ini_set('memory_limit', "128M");

    for ($y = 0; $y < $dataSizeInKB; $y++) {
        for ($x = 0; $x < 32; $x++) { //1kB
            $memData .= md5(time() + (($y * 32) + $x));
        }
    }

    file_put_contents("memdata.txt", $memData);

// This function creates a cyclic variable loop
    function useSomeMemory($x)
    {
        $data = [];
        $data[$x] = file_get_contents("memdata.txt");
        $data[$x + 1] = &$data;
    }

    for ($x = 0; $x < 1000; $x++) {
        useSomeMemory($x);
        if ($performGC == true) {
            gc_collect_cycles();
        }
    }

    printf("\nused: %10d | allocated: %10d | peak: %10d\n",
        memory_get_usage(),
        memory_get_usage(true),
        memory_get_peak_usage(true)
    );
}


// Code start...



$MB = 1024 * 1024;

$memLimitChecker = new MemoryLimitChecker();

$memLimitChecker->addLimit(8 * $MB, 'logFunction');
$memLimitChecker->addLimit(16 * $MB, 'logAndGC');
$memLimitChecker->addLimit(64 * $MB, 'abortOperation');

register_tick_function([$memLimitChecker, 'checkMemoryLimits']);

performMemIntensiveOperation(false);

exit(0);

