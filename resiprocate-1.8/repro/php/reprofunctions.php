<?php
/*
System:  Repro
File:    reprofunctions.php
Purpose: shared functions used by multiple pages
Author:  S. Chanin
*/
/* because I don't have a final database abstraction to work with and I don't 
know how to save db connection state in a cookie (or even if that's possible for
all the db's we need to support, I'm going to make every function open and close
it's own connection.  This is inefficient, but at least it's clean. */

// shared constants (the alternative for this would be to define them
// as constants
$provider = "XYZ";
$providerEmail = "XYZ Activation <activation@xyz.com>";
$sessionDuration = 600;  /* 600 seconds = 10 min */

/*
Purpose: Used for debugging.  Can pretty print a variable to the browser
  or can stuff the pretty printed version in a string (in Broswer format).

return values are:
  if $return_str is FALSE or not passed: ""
  if $return_str is TRUE: the printable representation of the $data
*/
function dbgShowBroswer($data, $return_str = false, $func = "print_r"){
   ob_start();
   $func($data);
   $output = '<pre>'.htmlspecialchars(ob_get_contents()).'</pre>';
   ob_end_clean();
   if($return_str) {
     return $output; 
   } else {
     echo $output;
     return("");
   }
}

/*
Purpose: Used for debugging.  Stuffs the pretty printed version in a string into
  a string which is returned so it can be stored in a file.

return values are:
  the storable representation of the $data
*/
function dbgShowFile($data, $func = "print_r"){
   ob_start();
   $func($data);
   $output = ob_get_contents();
   ob_end_clean();
   return $output; 
}


/*
Purpose: Used for debugging.  Appens a string ($) to the file /tmp/squirt.
  Use tail -f /tmp/squirt in a terminal window to watch the output.
*/
function dbgSquirt($s, $stamp=1) {
  $fp = fopen ("/tmp/squirt", "a+");
  if ($stamp == 1) {
    fputs($fp, date('ymd H:i:s '));
  }
  fputs($fp, $s."\n");
  fclose($fp);
}

/*
Purpose: checks if the supplied user/password combination matches a known user.

If so, the state of that user is returned.

return values are:
    A = matches an active user
    U = matches an unverified user
    D = matches a disabled user
    N = does not match */
function validateUser($u, $p) {
    $db = mysql_connect("localhost","apache","apache") or die(mysql_error());
    mysql_select_db("repro",$db) or die (mysql_error());
    $query="select * from Users where username='$u' and password='$p'";
    $result = mysql_query($query) or die(mysql_error());
    
    $count=mysql_num_rows($result);
    
    if ($count == 1) {
        // we matched, so lets get the state of the user
        $state = mysql_result($result,0,"state");
    } else {
		$state = "N"; }
		
    mysql_free_result($result);
	mysql_close($db);
	return $state;
}

/*
Purpose: Used to get the state of a user.  The state will only be returned if
the function is called with an activationCode that matches the one set for that
user in the database. 

return values are:
    A = matches an active user
    U = matches an unverified user
    D = matches a disabled user
    N = does not match */
function getUserState($user, $code) {
    $db = mysql_connect("localhost","apache","apache") or die(mysql_error());
    mysql_select_db("repro",$db) or die (mysql_error());
    $query="select * from Users where username='$user' and activationCode='$code'";
    $result = mysql_query($query) or die(mysql_error());
    
    $count=mysql_num_rows($result);
    
    if ($count == 1) {
        // we matched, so lets get the state of the user
        $state = mysql_result($result,0,"state");
    } else {
		$state = "N"; }
		
    mysql_free_result($result);
	mysql_close($db);
	return $state;
}



/*
Purpose: Check to see if a user name is already in use

return values are:
    Y = username is in use
    N = username is not in use

Note -- it is not possible to reuse a user name. */
function usernameInUse($u) {
    $db = mysql_connect("localhost","apache","apache") or die(mysql_error());
    mysql_select_db("repro",$db) or die (mysql_error());
    $query="select * from Users where username='$u'";
    $result = mysql_query($query) or die(mysql_error());
    
    $count=mysql_num_rows($result);
    if ($count == 1) {
        // we matched, so that name is in use
        $state = "Y";
    } else {
		$state = "N"; }
		
	mysql_free_result($result);
	mysql_close($db);
	return $state;
}

/*
Purpose: create a new account in the system.  New accounts are automatically
         created in U (unverified) state and have the current date used for 
	 the activationDate.

return values are:
	True = account creation succeeded.
	False = account creation failed. */
function createAccount($username, $passwordMD5, $fullname, $domain, $email,$activationCode) {
    $db = mysql_connect("localhost","apache","apache") or die(mysql_error());
    mysql_select_db("repro",$db) or die (mysql_error());
    $activationDate = date("Y-m-d");
    $query="insert into Users (username,password,fullname,domain,email,state,activationDate,activationCode) values('$username','$passwordMD5','$fullname','$domain','$email','U','$activationDate','$activationCode')";

    $result = mysql_query($query) or die(mysql_error());

	$count = mysql_affected_rows();
	
    if ((1 == $count) && (TRUE == $result)) {
        // no error and 1 row inserted
        $state = TRUE;
    } else {
		$state = FALSE; }
		
	mysql_close($db);
	return $state;
}

/*
Purpose: set a new account to active status

return values are:
	TRUE = account activation succeeded.
	FALSE = account activation failed. */
function activateUser($username, $activationCode) {
    $db = mysql_connect("localhost","apache","apache") or die(mysql_error());
    mysql_select_db("repro",$db) or die (mysql_error());
    $activationDate = date("Y-m-d");
    $query="update Users set state = 'A' where username = '$username' and activationCode = '$activationCode'";

    $result = mysql_query($query) or die(mysql_error());

	$count = mysql_affected_rows();
	
    if ((1 == $count) && (TRUE == $result)) {
        // no error and 1 row updated
        $state = TRUE;
    } else {
		$state = FALSE; }
		
	mysql_close($db);
	return $state;
}

/*
Purpose: Check to see if the supplied username and email address match a known
	 active user (can't do password resets for unverified or disabled 
	 users)

return values are:
    TRUE = username/email combination are a match
    FALSE = the combination does not match
*/
function matchUserAndEmail($username,$email) {
    $db = mysql_connect("localhost","apache","apache") or die(mysql_error());
    mysql_select_db("repro",$db) or die (mysql_error());
    $query="select * from Users where username='$username' and email='$email' and state = 'A'";
    $result = mysql_query($query) or die(mysql_error());
    
    $count=mysql_num_rows($result);
    if ($count == 1) {
        // we matched, so that user/email combination is valid
        $state = TRUE;
    } else {
		$state = FALSE; }
		
	mysql_free_result($result);
	mysql_close($db);
	return $state;
}

/*
Purpose: Create a new resource for a user.

return values are:
	TRUE = create succeeded.
	FALSE = create failed. */
function createResource($username, $aor, $forwardType, $forwardDestination, $voicemail) {
    $db = mysql_connect("localhost","apache","apache") or die(mysql_error());
    mysql_select_db("repro",$db) or die (mysql_error());

    // first we need to get the userid from the username
    $query="select id from Users where username = '$username'";
    $result = mysql_query($query) or die(mysql_error());

    $count=mysql_num_rows($result);
    if ($count == 1) {
      // we matched, so lets get the userid of the user
      $userid = mysql_result($result,0,"id");
      mysql_free_result($result);
        
      // if there are any constraints (e.g. AOR must be unique, etc, check
      // for them here
		
      // add the resource to the Resources table
      $query = "insert into Resources (userid,aor,forwardType,forwardDestination,voicemail) values($userid,'$aor','$forwardType','$forwardDestination','$voicemail')";
		
      $result = mysql_query($query) or die(mysql_error());
      $count = mysql_affected_rows();
	
      if ((1 == $count) && (TRUE == $result)) {
        // no error and 1 row inserted
	$state = TRUE;
      } else {
	$state = FALSE; }
    } else {
      $state = FALSE; }

    mysql_free_result($result);
    mysql_close($db);
    return $state;
}

/*
Purpose: Looks up other info tied to a user. 

Since arguments are passed by reference, they are set to the values returned
by the select.  The functions return value is used to indicate whether execution
succeed or failed.

return values are:
	TRUE == lookup suceeded.
	FALSE == error during lookup
*/

function lookupUserInformation($username,&$id,&$fullname,&$domain,&$email) {
    $db = mysql_connect("localhost","apache","apache") or die(mysql_error());
    mysql_select_db("repro",$db) or die (mysql_error());
    $query="select * from Users where username='$username'";
    $result = mysql_query($query) or die(mysql_error());
    
    $count=mysql_num_rows($result);
    
    if ($count == 1) {
        // we matched, so lets get the state of the user
        $id = mysql_result($result,0,"id");
        $fullname = mysql_result($result,0,"fullname");
        $domain = mysql_result($result,0,"domain");
        $email = mysql_result($result,0,"email");

        $state = TRUE;
    } else {
		$state = FALSE; }
		
    mysql_free_result($result);
	mysql_close($db);
	return $state;
}

/*
Purpose: Builds an associative array containing all the resources associated
		 with a username.  This is extra work, but it should isolate any dependency
		 on mysql here and allow the function to be re-implemented for other
		 databases without affecting the surrounding code.
		 
return values are:
	TRUE == lookup succeeded
	FALSE == lookup failed
*/
function getResourcesByUsername($username,&$resources) {
    $db = mysql_connect("localhost","apache","apache") or die(mysql_error());
    mysql_select_db("repro",$db) or die (mysql_error());

    // first we need to get the userid from the username
    $query="select id from Users where username = '$username'";
    $result = mysql_query($query) or die(mysql_error());

    $count=mysql_num_rows($result);
    // print "Query -- $query<br />\nCount -- $count<br \>\n";
    
    if ($count == 1) {
        // we matched, so lets get the userid of the user
        $userid = mysql_result($result,0,"id");
        mysql_free_result($result);
        
        $query = "select id,aor,forwardType,forwardDestination,voicemail from Resources where userid = '$userid'";
        $result = mysql_query($query) or die(mysql_error());
        
        // print "Query -- $query<br />\nResult -- $result<br />\n";

		$state = TRUE;
		while (($myrow = mysql_fetch_array($result))) {
			// print "Row -- ";
			// print_r($myrow);
			$newRow = array($myrow['id'],$myrow['aor'],$myrow['forwardType'],$myrow['forwardDestination'],$myrow['voicemail']);

			// print "<br />New Row --";
			// print_r($newRow);
			$resources[] = $newRow;
			// print "<br />Resource -- ";
			// print_r($resources);
		}
    } else {
    	$state = FALSE;
   	}
    
    mysql_free_result($result);
	mysql_close($db);
	return $state;
}

/*
Purpose: gets the shared salt from the database to use in creating authentication
tokens.

return values are:
	TRUE == salt successfully retrieved
	FALSE == error while retreiving salt
*/
function getSalt(&$salt) {
    $db = mysql_connect("localhost","apache","apache") or die(mysql_error());
    mysql_select_db("repro",$db) or die (mysql_error());
    $query="select value from Parameters where parameter='salt'";
    $result = mysql_query($query) or die(mysql_error());
    
    $count=mysql_num_rows($result);
    
    if ($count == 1) {
        // we matched, so lets get the state of the user
        $salt = mysql_result($result,0,"value");
        $state = TRUE;
    } else {
		$salt = ""; 
		$state = FALSE; }
		
    mysql_free_result($result);
	mysql_close($db);
	return $state;
}

/*
Purpose: clears authentication cookies

return values are:
	TRUE == no errors reported from setcookie
	FALSE == errors were reported
*/
function clearCookies() {

dbgSquirt("==============Function: Clear Cookies ==============");
dbgSquirt('Cookie --' . dbgShowFile($_COOKIE));


  $result = setcookie("user","",mktime(12,0,0,1,1,1970));
  $result1 = setcookie("authentication","",mktime(12,0,0,1,1,1970));

  return ($result && $result1);
}

/*
Purpose: checks whether the current cookies validate the user or if additional
         authentication is needed.

	 if the cookies are unset or are blank, $ignoreBlanks is checked.
	 if $ignoreBlanks is TRUE, no error is reported in this case.
	 if $ignoreBlanks is FALSE, then this case is treated as an error.
	 ...in either case, blank or unset cookies will result in $forceLogin
	    being true.

return values are:
	TRUE == no errors reported
	FALSE == errors were reported

mutates the following:
	$forceLogin: TRUE == cookies contain valid authentication data
	             FALSE == user is NOT authenticated
	$error: "" == no errors
	        otherwise contains displayable text of error
*/
function checkCookies(&$forceLogin,&$error,$ignoreBlanks) {
  $forceLogin = TRUE;
  $error = "";
  global $sessionDuration;

  dbgSquirt("==============Function: checkCoookies ==============");
  dbgSquirt('Cookie --' . dbgShowFile($_COOKIE));

  if (isset($_COOKIE['user']) && !empty($_COOKIE['user']) &&
      isset($_COOKIE['authentication']) && !empty($_COOKIE['authentication'])) {
    // both user and authentication cookies are set and non-blank
    // dbgSquirt("Cookies set and non-empty");
    $userCookie = $_COOKIE['user'];
    $authenticationCookie = $_COOKIE['authentication'];
    $time = time();

    // dbgSquirt("Getting salt");
    if (getSalt($salt)) {
      // dbgSquirt("...salt gotten");
      // dbgSquirt("Encrypting");
      if (sha1($userCookie . $salt) == $authenticationCookie) {
	// authentication passed
	// so reset expiration on cookies
	// dbgSquirt("Cookie matches encryption");
	// dbgSquirt("Resetting cookies");
	// dbgSquirt("Time -- $time");
	// dbgSquirt("Time + Duration -- ". ($time+$sessionDuration));
	$result = setcookie("user",$userCookie,$time+$sessionDuration);
	$result1 = setcookie("authentication",$authenticationCookie,
			     $time+$sessionDuration);
	if ((TRUE == $result) && (TRUE == $result1)) {
	  // everything worked
	  // dbgSquirt("Everything worked ... no need to forceLogin");
	  $forceLogin = FALSE;
	} else {
	  $error = "Internal error -- problem while creating cookies.  Please contact an administrator.";
	}
      } else {
	// credentials in cookies don't match.
	// dbgSquirt("Cookie does NOT match encryption");
	$error = "Authentication error -- The supplied credentials don't match our stored values. Please reauthenticate and try again.";
      }
    } else {
      // dbgSquirt("...error while getting salt");
      // error while trying to get salt value
      $error = "Internal error -- unable to validate supplied credentials. Please reauthenticate and try again.";
    }
  } else {
    // cookies were unset or contained empty values
    // dbgSquirt("Cookies unset or empty");
    if (FALSE == $ignoreBlanks) {
      $error = "Please log in."; }
  }

  dbgSquirt("Returning -- ". empty($error));
  return(empty($error));
}

/*
Purpose: change the fullname saved for a user

return values are:
	TRUE = change succeeded.
	FALSE = change failed. */
function updateFullname($username, $newFullname) {
    $db = mysql_connect("localhost","apache","apache") or die(mysql_error());
    mysql_select_db("repro",$db) or die (mysql_error());
    $query="update Users set fullname = '$newFullname' where username = '$username'";

    $result = mysql_query($query) or die(mysql_error());

    $count = mysql_affected_rows();
	
    if ((1 == $count) && (TRUE == $result)) {
        // no error and 1 row updated
      $state = TRUE;
    } else {
      $state = FALSE; }
		
    mysql_close($db);
    return $state;
}

/*
Purpose: Create an encrypted password based on the username and supplied 
         cleartext password.

Returns encrypted password */
function createPassword($username, $password) {
  $encryptedPassword = md5($username . "::" . $password);
  return $encryptedPassword;
}

/*
Purpose: change the password saved for a user

Note:    expects the password to come in already encrypted

return values are:
	TRUE = change succeeded.
	FALSE = change failed. */
function updatePassword($username, $newPassword) {
  dbgSquirt("============= Function: updatePassword ===========");

  $db = mysql_connect("localhost","apache","apache") or die(mysql_error());
  mysql_select_db("repro",$db) or die (mysql_error());
  $query="update Users set password = '$newPassword' where username = '$username'";
  dbgSquirt("Query -- $query");

  $result = mysql_query($query) or die(mysql_error());
    
  $count = mysql_affected_rows();
	
  if ((1 == $count) && (TRUE == $result)) {
    // no error and 1 row updated
    $state = TRUE;
  } else {
    $state = FALSE; }
		
  mysql_close($db);
  return $state;
}

/*
Purpose: change the email saved for a user

return values are:
	TRUE = change succeeded.
	FALSE = change failed. */
function updateEmail($username, $newEmail) {
  dbgSquirt("============= Function: updateEmail ===========");

  $db = mysql_connect("localhost","apache","apache") or die(mysql_error());
  mysql_select_db("repro",$db) or die (mysql_error());
  $query="update Users set email = '$newEmail' where username = '$username'";
  dbgSquirt("Query -- $query");

  $result = mysql_query($query) or die(mysql_error());
    
  $count = mysql_affected_rows();
	
  if ((1 == $count) && (TRUE == $result)) {
    // no error and 1 row updated
    $state = TRUE;
  } else {
    $state = FALSE; }
		
  mysql_close($db);
  return $state;
}

/*
Purpose: Delete a resource

Note: to limit risk this function makes sure the resourceId that is being
  flagged for deletion is owned by the user passed in (which should be the
  username from the authentication cookies)

return values are:
	TRUE = delete succeeded.
	FALSE = delete failed. */
function deleteResource($username, $resourceId) {
  dbgSquirt("============= Function: deleteResource ===========");

  $db = mysql_connect("localhost","apache","apache") or die(mysql_error());
  mysql_select_db("repro",$db) or die (mysql_error());

  // first we need to get the userid from the username
  $query="select id from Users where username = '$username'";
  dbgSquirt("Query -- $query");
  $result = mysql_query($query) or die(mysql_error());

    $count=mysql_num_rows($result);
    dbgSquirt("Rows -- $count");
    if ($count == 1) {
      // we matched, so lets get the userid of the user
      $userid = mysql_result($result,0,"id");
      mysql_free_result($result);
        
      // delete the resource
      $query = "delete from Resources where userid = '$userid' and id = '$resourceId'";
      dbgSquirt("Query2 -- $query");

      $result = mysql_query($query) or die(mysql_error());
      $count = mysql_affected_rows();

      dbgSquirt("Rows -- $count");	
      if ((1 == $count) && (TRUE == $result)) {
        // no error and 1 row deleted (should only be 1 row since id is
	// the primary key)
	$state = TRUE;
      } else {
	$state = FALSE; }
    } else {
      $state = FALSE; }

    mysql_free_result($result);
    mysql_close($db);
    return $state;
}

/*
Purpose: update a resource based on the resourceId

return values are:
	TRUE = update succeeded.
	FALSE = update failed. */
function updateResource($resourceId,$username,$resource,$forwardType,
			$forward,$voicemail) {
  dbgSquirt("============= Function: updateResource ===========");

  $db = mysql_connect("localhost","apache","apache") or die(mysql_error());
  mysql_select_db("repro",$db) or die (mysql_error());

  // first we need to get the userid from the username
  $query="select id from Users where username = '$username'";
  dbgSquirt("Query -- $query");
  $result = mysql_query($query) or die(mysql_error());

  $count=mysql_num_rows($result);
  dbgSquirt("Rows -- $count");
  if ($count == 1) {
    // we matched, so lets get the userid of the user
    $userid = mysql_result($result,0,"id");
    mysql_free_result($result);
        
    // delete the resource
    $query = "update Resources set aor='$resource',forwardType='$forwardType',forwardDestination='$forward',voicemail='$voicemail' where userid = '$userid' and id = '$resourceId'";
    dbgSquirt("Query2 -- $query");

    $result = mysql_query($query) or die(mysql_error());
    $count = mysql_affected_rows();

    dbgSquirt("Rows -- $count");	
    if ((1 == $count) && (TRUE == $result)) {
      // no error and 1 row modified (should only be 1 row since id is
      // the primary key)
      $state = TRUE;
    } else {
      $state = FALSE; }
  } else {
    $state = FALSE; }

  mysql_free_result($result);
  mysql_close($db);
  return $state;
}
?>
