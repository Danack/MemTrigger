<?php


namespace trigger {

	function init($ticks)
	{
	}

	/**
	 * @param int $triggerBytes
	 * @param callable $callback
	 */
	function register(\Trigger $trigger)
	{
	}
}

namespace {
	/**
	 * @param int $ticks
	 */

	class TriggerException extends Exception
	{
	}


	class Trigger
	{
		const ACTION_LEAVE_ACTIVE = 0;
		const ACTION_DISABLE = 1;

		const STATE_ACTIVE = 0;
		const STATE_DISABLED = 1;

		function __construct(callable $callback, $value, $action, $state = 0)
		{

		}

		/**
		 * @param $newValue
		 */
		function setTriggerValue($newValue)
		{
		}

		function getTriggerValue()
		{
			return 1;
		}

		function setResetValue($newValue)
		{
		}

		function getResetValue()
		{
			return 1024;
		}

		function setState($newState)
		{
		}

		function getState()
		{
			return 1;
		}
	}
}