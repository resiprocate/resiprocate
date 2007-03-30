<?php

require('reprofunctions.php');
dbgSquirt("============= userhome ===============");

// TODO
/* There is a bug here ... if a user has authenticated successfully (and hence
	the cookies for username and passwordMD5 are set) and then they use BACK
	to go back to the login page, enter some values for username and 
	password, and click login, then what they just typed will be ignored, 
	and they will remain logged in under their original credentials. */

// this variable controls whether the user is forced back to the main page to
// login.  For safety, the default value is to force you back.
$forceLogin = TRUE;
$error = "";
$time = time();

if (!checkCookies($forceLogin,$error,TRUE)) {
  // we got an error back that occurred while checkCookies was being run
  dbgSquirt('Error from checkCookies');
  header("Location: http://" . $_SERVER['HTTP_HOST'] . 
	 dirname($_SERVER['PHP_SELF']) . "/index.php?error=$error");
  exit;
 }

// if the cookie's didn't pass authentication, or if the cookie's passed BUT
// we've received new values for POST that don't match on username (they did
// a BACK to the login page w/o a logout and then did a new login), then
// try to authenticate via the POSTED values been supplied.
if (isset($_POST['username']) && ($_POST['username'] != $_COOKIE['user']))
  $forceLogin = TRUE;

if ($forceLogin) {
  dbgSquirt('forceLogin is still true... checking post variables');
  if (isset($_POST['username']) || isset($_POST['password'])) {
    // we have one or more post variables
    dbgSquirt('Post variables are set');
    if (empty($_POST['username']) || empty($_POST['password'])) {
      // can't have empty values for username or password
      dbgSquirt('...but one is empty');
      $error = "Authentication error -- you must enter a username and password.";
    } else {
      // we have non-empty values for username and password from POST so
      // lets validate them
      dbgSquirt('...both are non-empty [good]');
      $username = $_POST['username'];
      $password = $_POST['password'];
      $encryptedPassword = createPassword($username,$password);

      $state = validateUser($username,$encryptedPassword);
      if ("N" == $state) {
	dbgSquirt('Not a valid user');
	$error = "Authentication error -- Invalid username/password combination.";
      } else if ("A" == $state) {
	// active account and username/password match
	dbgSquirt('Active account matched.');

	// if we haven't already looked up the salt, do so now
	$result = TRUE;
	if (empty($salt)) {
	  dbgSquirt('Getting salt');
	  $result = getSalt($salt); }

	if (FALSE == $result) {
	  // uh-oh ... we got an error getting the salt
	  dbgSquirt('Error in getSalt');
	  $error = "Internal error -- failure while processing login. Please contact an administrator.";
	} else {
	  dbgSquirt('Extending cookies');
	  dbgSquirt("Time -- $time");
	  dbgSquirt("Time + Duration -- ". ($time+$sessionDuration));
	  $result = setcookie("user",$username,$time+$sessionDuration);
	  $result1 = setcookie("authentication",sha1($username . $salt),
			       $time+$sessionDuration);

	  if ((TRUE == $result) && (TRUE == $result1)) {
	    // everything worked
	    dbgSquirt('Everything worked.');
	    $forceLogin = FALSE;
	  } else {
	    dbgSquirt('Error while creating cookies');
	    $error = "Internal error -- problem while creating cookies.  Please contact an administrator.";
	  }
	}
      } else if ("U" == $state) {
	// unverified account
	dbgSquirt('Unverified Account');
	$error="This account has not been verified. Please check for the verification email you were sent as part of the signup process.";
      } else if ("D" == $state) {
	// disabled account
	dbgSquirt('Disabled Account');
	$error = "This account has been disabled.";
      } else {
	// should not happen ... checked return value from validateUser
	dbgSquirt('Unknown return code from validateUser');
	$error = "Internal Error -- error validating username/password.  Please try again.  This this error reoccurs, please contact an administrator.";
	  }
    }
  } else {
    // no post variables supplied
    dbgSquirt('No post variables');
    $error = "Authentication error -- you must enter a username and password.";
  }
 } else {
  // forceLogin was FALSE ... that means the cookie's were valid
  // so get username from the cookie
  $username = $_COOKIE['user'];
 }

// after checking cookies and post variables, if a login is still needed, then
// redirect
dbgSquirt("After post check -- forceLogin = $forceLogin");
if ($forceLogin) {
  header("Location: http://" . $_SERVER['HTTP_HOST'] . 
            dirname($_SERVER['PHP_SELF']) . 
            "/index.php?error=$error");
  exit;
 }
?>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<!--
System:  Repro
File:    userhome.php
Purpose: User Home Page.  This displays the users personal information and
         allows changes to be made.
Author:  S. Chanin
-->

<html>
<head>
<link rel="stylesheet" type="text/css" href="repro_style.css" />
<title></title>
</head>

<body>
<h1 class="title">Repro</h1>

<h1>User Home Page</h1>
<hr />

<?php
// if we've looped back due to an error, show the message
if (isset($_GET["error"]) && !empty($_GET['error'])) {
    echo '<p class="error">' . $_GET["error"] . "</p>\n";
}
if (!lookupUserInformation($username,$id,$fullname,$domain,$email)) {
	echo "<h2>Error -- Error while accessing account information</h2>\n";
	echo "<p>Please contact your administrator for assistance.</p>\n";
} else {
?>
<!-- show basic user information with the user -->
<table border="0" cellpadding="5">
<tr>
<td>Username</td>
<td><h2><?php echo $username ?></h2></td>
</tr>

<tr>
<td>Fullname</td>
<td><?php echo $fullname ?></td>
<td><a href="changefullname.php">Change Fullname</a></td>
</tr>

<tr>
<td>Password</td>
<td>********</td>
<td><a href="changepassword.php">Change Password</a></td>
</tr>

<tr>
<td>Email</td>
<td><?php echo $email ?></td>
<td><a href="changeemail.php">Change Email</a></td>
</tr>

<tr>
<td>Domain</td>
<td><?php echo $domain ?></td>
</tr>
</table>

<!-- now show the resources associated with the user -->
<br />
<table border="1">
<th class="header">Address</th><th class="header">Forward</th>
<th class="header">Voicemail</th><th class="header">Edit</th><th class="header">Delete</th>

<?php
$result = getResourcesByUsername($username,$resources);
// print "<br />Final Result --";
// print_r($resources);
foreach ($resources as $r) {
  // print "Row -- ";
  // print_r($r);
  // print "<br />";
	
  $id = $r[0];
  $aor = $r[1];
  $forwardType = $r[2];
  $forward = $r[3];
  $voicemail = $r[4];

  echo "<tr>";
  echo '<form method="post" action="modifyresource.php">'."\n";
  echo "<td>$aor</td>\n";
  if ("Y" == $forwardType)
    echo "<td>$forward</td>\n";
  else
    echo "<td>&nbsp</td>\n";
  echo "<td>$voicemail</td>\n";

  echo '<td><input type="submit" name="edit" id="edit" value="Edit"/></td>'."\n";
  echo '<td><input type="submit" name="delete" id="delete" value="Delete"/></td>'."\n";
  echo '<input type="hidden" name="resourceId" id="resourceId" value="' . $id .'" />'."\n";
  echo '<input type="hidden" name="aor" id="aor" value="' . $aor .'" />'."\n";
  echo '<input type="hidden" name="forwardType" id="forwardType" value="' . $forwardType .'" />'."\n";
  echo '<input type="hidden" name="forward" id="forward" value="' . $forward .'" />'."\n";
  echo '<input type="hidden" name="voicemail" id="voicemail" value="' . $voicemail .'" />'."\n";
  echo "</form>\n";
  echo "</tr>\n";
}
?>
</table>
<form method="post" action="addresource.php">
<input type="submit" name="addResource" id="addResource" value="Add Resource" />
</form>

<?php
} ?>
<br /><hr /><a href="logout.php">Logout</a>

</body>
</html>
