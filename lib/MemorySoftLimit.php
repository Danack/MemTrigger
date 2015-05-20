<?php

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