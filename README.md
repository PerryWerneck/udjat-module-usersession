# User/Session monitor and library for udjat.

[![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![CodeQL Advanced](https://github.com/PerryWerneck/libudjatusers/actions/workflows/codeql.yml/badge.svg)](https://github.com/PerryWerneck/libudjatusers/actions/workflows/codeql.yml)
[![build result](https://build.opensuse.org/projects/home:PerryWerneck:udjat/packages/libudjatusers/badge.svg?type=percent)](https://build.opensuse.org/package/show/home:PerryWerneck:udjat/libudjatusers)

Watch active users sessions emitting UDJAT alerts (http call, script, etc) based on user's session events.

## Using agent

### Event names

 * *Already active*: Session is active on startup
 * *Still active*: Session still active on shutdown
 * *Login*: User has logged in
 * *Logout*: User has logged out
 * *Lock*: Session was locked
 * *Unlock*: Session was unlocked
 * *Foreground*: Session is in foreground
 * *Background*: Session is in background
 * *sleep*: Session is preparing to sleep
 * *resume*: Session is resuming from sleep
 * *shutdown*: Session is shutting down
 * *pulse*: Session is alive

### Examples

[Udjat](../../../udjat) service configuration to emit an alert on user logoff:

```xml
<?xml version="1.0" encoding="UTF-8" ?>
<config log-debug='yes' log-trace='yes'>

	<!-- The HTTP module implements the http client backend -->
	<module name='http' required='yes' />
	
	<!-- Implements user's monitor -->
	<module name='users' required='yes' />
	
	<!-- Declare an user monitor agent -->
	<agent type='users' name='users' update-timer='60'>

		<!-- Do an HTTP post when user screen is locked -->
		<script type='url' name='lock' trigger-event='lock' action='post' url='http://localhost'>
			{"user":"${username}","macaddress":"${macaddress}"}
		</script>

	</agent>
	
</config>
```

