<?php

/**
 * @param int $ticks 
 */
function memtrigger_init($ticks) {}


/**
 * @param int $triggerBytes
 * @param callable $callback
 */
function memtrigger_register(MemTrigger $memTrigger) {}


class MemTrigger {

	const ACTION_LEAVE_ACTIVE = 0;
	const ACTION_DISABLE = 1;

	const STATE_ACTIVE = 0;
	const STATE_DISABLED = 1;

	const TYPE_ABOVE = 0;
	const TYPE_BELOW = 1;

	
	function __construct(callable $callback, $value, $action, $type = 0, $state = 0)
	{

	}
	
	/**
	 * @param $newValue
	 */
	function setTriggerValue($newValue) {}
	function getTriggerValue() {return 1;}

	function setResetValue($newValue) {}
	function getResetValue() {return 1024;}

	function setState($newState) {}
	function getState() {return 1;}

	function setTriggerAction($newTriggerAction) {}
	function getTriggerAction() {return 1;}
}