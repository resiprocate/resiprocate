<?php
require('reprofunctions.php');
dbgSquirt("============= Add Resource ===============");

$result = checkCookies($forceLogin,$error,FALSE);
if (!($result) || $forceLogin) {
  // we got an error back that occurred while checkCookies was being run, 
  // or authentication failed.  Either way, bounce them back to the login screen
  header("Location: http://" . $_SERVER['HTTP_HOST'] . 
	 dirname($_SERVER['PHP_SELF']) . 
	 "/index.php?error=$error");
  exit;
 }
$username = $_COOKIE['user'];
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<!--
System:  Repro
File:    addresource.php
Purpose: Allows an authenticated user to add additional resources to their
         profile
Author:  S. Chanin
-->
<html>
<head>
<link rel="stylesheet" type="text/css" href="repro_style.css" />
<title>Add Resource</title>
<script type="text/javascript">
<!--
function disableForward() {
  document.resourceForm.forward.value = ""
  document.resourceForm.forward.disabled = true
}

function enableForward() {
  document.resourceForm.forward.disabled = false
}
//-->
</script>
</head>

<body>
<h1 class="title">Repro</h1>
<h1>Add Resource</h1>
<hr />

<?php
// if we've looped back due to an error, show the message
if (isset($_GET["error"]) && !empty($_GET['error'])) {
    echo '<p class="error">' . $_GET["error"] . "</p>\n";
}
?>
<form method="POST" action="savenewresource.php" name="resourceForm" id="resourceForm">
<table>
<tr>
<td>Address</td>
<td><input type="text" name="aor" id="aor" value="<?php echo $_GET['aor']; ?>"/></td>
</tr>
<tr>
<td>Forward</td>
    <td><input type="radio" name="forwardType" id="forwardType" value="Yes" 
    onclick="enableForward()" 
    <?php if (!isset($_GET['forwardType']) || $_GET['forwardType'] == "Yes")
           echo 'checked="checked"'; ?>
    >Yes</td>
</tr>
<tr><td>&nbsp</td>
<td><input type="radio" name="forwardType" id="forwardType" value="No" 
       onclick="disableForward()"
       <?php if ($_GET['forwardType'] == "No") echo 'checked="checked"'; ?>
       >No</td></tr>
<tr><td>Forward Address</td>
<td><input type="text" name="forward" id="forward" value="<?php echo $_GET['forward']; ?>"/>
</td>
</tr>
<tr>
<td>Voicemail</td>
<td><input type="text" name="voicemail" id="voicemail" value="<?php echo $_GET['voicemail']; ?>"/></td>
</tr>
<tr>
<td>&nbsp</td>
<td>
<input type="submit" name="submit" id="submit" value="Save" />
<input type="submit" name="submit" id="submit" value="Cancel" />
</td>
</tr>
</table>
</form>

<br /><hr />
<a href="userhome.php">Return to User Home</a><br />
<a href="logout.php">Logout</a><br />
</body>
</html>
