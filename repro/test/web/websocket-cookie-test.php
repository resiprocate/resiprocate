<?

  // This code demonstrates how to set a cookie that can be
  // checked by the WebSocket cookie authentication scheme

  if( isset($_POST['softphone_link']))
  {
    $softphone_link = $_POST['softphone_link'];
    $proxy_domain = $_POST['proxy_domain'];
    $sip_from = $_POST['sip_from'];
    $sip_to = $_POST['sip_to'];
    $hmac_key = $_POST['hmac_key'];
    $time_limit = $_POST['time_limit'];
    $cookie_value = '1:' . time() . ':' . $time_limit . ':' . $sip_from . ':' . $sip_to;
    $extra_value = $_POST['extra_value'];
    $digest_input = $cookie_value . ':' . $extra_value;
    $cookie_mac = hash_hmac ( 'sha1', $digest_input, $hmac_key);
    // not sure why setcookie didn't work, we use setrawcookie instead:
    setrawcookie("WSSessionInfo", urlencode($cookie_value), $time_limit, '/', $proxy_domain);
    setrawcookie("WSSessionExtra", urlencode($extra_value), $time_limit, '/', $proxy_domain);
    setrawcookie("WSSessionMAC", $cookie_mac, $time_limit, '/', $proxy_domain);
  }

?>
<html>
<body>
  <a href="<? echo $softphone_link; ?>"><? echo $softphone_link; ?></a><br/>
  <form action="<?php $_PHP_SELF ?>" method="POST"><br/>
  Softphone (used in HTTP redirect): <input type="text" name="softphone_link" value="http://tryit.jssip.net"/><br/>
  Domain part of "ws:" URI, used in setting cookie domain: <input type="text" name="proxy_domain" value=""/><br/>
  From: <input type="text" name="sip_from" value=""/><br/>
  To: <input type="text" name="sip_to" value=""/><br/>
  HMAC key (must match repro.config WSCookieAuthSharedSecret): <input type="text" name="hmac_key" value=""/><br/>
  Timeout (default now + 1000s): <input type="text" name="time_limit" value="<? echo (time()+1000); ?>"/><br/>
  Extra header value: <input type="text" name="extra_value" value=""/><br/>
  <input type="submit" /><br/>
  <p>Note: for from and to URIs, you may use "*" to replace the user, host or both, e.g. *@example.org would allow calls to anybody in example.org</p>
  </form>
  <h3>Usage notes</h3>
  <ul>
    <li>enable a WS transport and WSCookieAuthSharedSecret in repro.config</li>
    <li>browse to this PHP page</li>
    <li>set proxy_domain to exactly match the domain used by the WS transport</li>
    <li>NOTE: proxy_domain must also match the domain where this page is hosted otherwise the browser doesn't seem to accept the cookie.
      <ul>
        <li>You can strip the host portion of the domain, </li>
        <li>host the page on www.example.org,</li>
        <li>proxy ws://sip.example.org</li>
        <li>use proxy_domain = example.org to serve both</li>
      </ul>
    </li>
    <li>set sip_from and sip_to to restrict the callee and caller URIs, or just use wildcards like *@*</li>
    <li>set the HMAC key to match the key in repro.config</li>
    <li>make sure you connect within the specified time_limit</li>
  </ul>
  <p>For a full overview, <a href="http://www.resiprocate.org/SIP_Over_WebSocket_Cookies">see the cookie page on the reSIProcate wiki</a></p>
</body>
</html>
