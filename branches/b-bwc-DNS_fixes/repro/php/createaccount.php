<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<!--
System:  Repro
File:    createaccount.php
Purpose: Collect information necessary to enroll a new user.
Author:  S. Chanin
-->

<html>
<head>
<link rel="stylesheet" type="text/css" href="repro_style.css" />
<title>Create Account</title>
</head>

<body>
<h1 class="title">Repro</h1>
<h1>Create Account</h1>
<hr />

<!-- if we've looped back due to an error, show the message -->
<?php
if (isset($_GET["error"])) {
	echo '<p class="error">' . $_GET["error"] . "</p>\n";
}
?>

<?php
// create an image of a random string of characters to use to ensure that a person
// is creating the account.

$image = imagecreate(120,30);

$white = imagecolorallocate($image,0xFF, 0xFF, 0xFF);
$gray = imagecolorallocate($image, 0xC0, 0xC0, 0xC0);
$darkgray = imagecolorallocate($image, 0x50, 0x50, 0x50);

srand((double)microtime()*1000000);

for ($i=0; $i<30; $i++) {
	$x1 = rand(0,120);
	$y1 = rand(0,30);
	$x2 = rand(0,120);
	$y2 = rand(0,30);
	
 	imageline($image, $x1, $y1, $x2, $y2, $gray);
}

for ($i=0; $i < 5; $i++) {
	$type = rand(0,2);
	
	if (0 == $type) {
		// generate a lower case letter
		$cnum[$i] = chr(rand(97,122));
	} else if (1 == $type) {
		// generate an upper case letter
		$cnum[$i] = chr(rand(65,90));
	} else {
		// generate a number
		$cnum[$i] = chr(rand(48,57));
	}
}

$x = 0;
for ($i=0; $i<5; $i++) {
	$fnt = rand(3,5);
	$x = $x + rand(12,20);
	$y = rand(7,12);

	imagestring($image,$fnt, $x, $y, $cnum[$i], $darkgray);
}

$string = "$cnum[0]$cnum[1]$cnum[2]$cnum[3]$cnum[4]";
$stringMD5 = md5($string);
$stringfile = "images/" . $stringMD5 . ".pgn";

imagepng($image,$stringfile);
imagedestroy($image);

?>

<form method="POST" action="emailactivationnotice.php">
<table>
<tr>
<td>Username</td>
<td><input type="text" id="username" name="username" value="<?php echo $_GET["username"]?>" /></td>
</tr>
<tr>
<td>Password</td>
<td><input type="password" id="password" name="password" /></td>
</tr>
<tr>
<td>Retype Password</td>
<td><input type="password" id="password2" name="password2" /></td>
</tr>
<tr>
<td>Full Name</td>
<td><input type="text" id="fullname" name="fullname" value="<?php echo $_GET["fullname"]?>"/></td>
</tr>
<tr>
<td>Domain</td>
<td><input type="text" id="domain" name="domain" readonly="readonly" value="XXX" class="readonly"/></td>
</tr>
<tr>
<td>Email</td>
<td><input type="text" id="email" name="email" value="<?php echo $_GET["email"]?>"/></td>
</tr>
<tr>
<td>Retype Email</td>
<td><input type="text" id="email2" name="email2" value="<?php echo $_GET["email2"]?>" /></td>
</tr>
<tr>
<td>Retype the characters displayed below</td>
</tr>
<tr>
<td><img width=120 height=30 src="<?php echo $stringfile ?>" border="1" /></td>
<td><input type="text" size="5" maxlength="5" name="userkey" id="userkey" value=""/>
	<input type="hidden" name="keyvalue" id="keyvalue" value="<?php echo $stringMD5 ?>" />
</td>
</tr>
<tr>
<td>&nbsp</td>
<td align="right">
  <input type="submit" id="submit" name="submit" value="Create" />
  <input type="reset" id="reset" name="reset" value="Reset" />
</td>
<tr>
</table>
</form>
<hr />
<a href="index.php">Return to Welcome Page</a>
<br />

</body>
</html>
