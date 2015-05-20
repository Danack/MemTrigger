<?php



/**
 * @param int $ticks 
 */
function memtrigger_init($ticks) {}


/**
 * @param int $triggerBytes
 * @param callable $callback
 */
function memtrigger_register(callable $callback, $triggerBytes, $resetBytes = 0, $disableSetting = 1) {}