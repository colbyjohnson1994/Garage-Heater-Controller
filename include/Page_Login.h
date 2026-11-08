const char PAGE_Login[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Login - Garage Heater</title>
<link rel="icon" href="/favicon.svg" type="image/svg+xml">
<link rel="stylesheet" href="/style.css">
</head>
<body>
<div class="container" style="max-width:400px; margin-top:60px;">
  <h1 style="text-align:center;">🔥 Garage Heater</h1>
  <div class="card">
    <form action="/login" method="post">
      <label>Username</label>
      <input type="text" name="user" required autocomplete="username" style="max-width:100%;">
      <label>Password</label>
      <input type="password" name="pass" required autocomplete="current-password" style="max-width:100%;">
      <button type="submit" class="btn" style="width:100%; margin-top:16px;">Sign In</button>
    </form>
    <p id="msg" style="color:#ef4444; text-align:center; margin-top:12px;"></p>
  </div>
</div>
<script>
if (location.search.includes('err')) document.getElementById('msg').textContent = 'Invalid username or password';
</script>
</body>
</html>
)=====";

void handleLogin()
{
  if (server.method() == HTTP_POST)
  {
    String user = server.arg("user");
    String pass = server.arg("pass");
    user.trim();
    pass.trim();

    uint32_t h = simpleHash(pass);
    Serial.printf("Login attempt: user='%s' (expected '%s') hash=%lu (expected %lu)\n",
                  user.c_str(), config.loginUser.c_str(), h, config.loginHash);

    if (user == config.loginUser && h == config.loginHash)
    {
      String token = String(random(0x100000, 0xFFFFFF), HEX) + String(millis(), HEX);
      config.sessionToken = token;
      server.sendHeader("Set-Cookie", "session=" + token + "; Path=/; HttpOnly; SameSite=Lax; Max-Age=86400");
      server.sendHeader("Location", "/", true);
      server.send(302, "text/plain", "");
      Serial.println("Login OK");
      return;
    }
    Serial.println("Login FAILED");
    server.sendHeader("Location", "/login?err=1", true);
    server.send(302, "text/plain", "");
    return;
  }
  server.send(200, "text/html", PAGE_Login);
}

void handleLogout()
{
  config.sessionToken = "";
  server.sendHeader("Set-Cookie", "session=; Path=/; HttpOnly; Max-Age=0");
  server.sendHeader("Location", "/login", true);
  server.send(302, "text/plain", "");
}

bool requireAuth()
{
  if (!isAuthenticated())
  {
    server.sendHeader("Location", "/login", true);
    server.send(302, "text/plain", "");
    return false;
  }
  return true;
}

const char PAGE_ChangePassword[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Change Password - Garage Heater</title>
<link rel="icon" href="/favicon.svg" type="image/svg+xml">
<link rel="stylesheet" href="/style.css">
</head>
<body>
<div class="container" style="max-width:420px; margin-top:40px;">
  <h1>Change Password</h1>
  <div class="card">
    <form action="/changepass" method="post">
      <label>Current Password</label>
      <input type="password" name="current" required style="max-width:100%;">
      <label>New Password</label>
      <input type="password" name="newpass" required minlength="4" style="max-width:100%;">
      <label>Confirm New Password</label>
      <input type="password" name="confirm" required minlength="4" style="max-width:100%;">
      <button type="submit" class="btn" style="width:100%; margin-top:16px;">Update Password</button>
    </form>
    <p id="msg" style="text-align:center; margin-top:12px;"></p>
  </div>
  <div class="nav"><a href="/">← Dashboard</a></div>
</div>
<script>
const params = new URLSearchParams(location.search);
const msg = document.getElementById('msg');
if (params.get('ok')) { msg.style.color = '#22c55e'; msg.textContent = 'Password updated'; }
if (params.get('err') === 'mismatch') { msg.style.color = '#ef4444'; msg.textContent = 'New passwords do not match'; }
if (params.get('err') === 'wrong') { msg.style.color = '#ef4444'; msg.textContent = 'Current password is incorrect'; }
if (params.get('err') === 'short') { msg.style.color = '#ef4444'; msg.textContent = 'Password must be at least 4 characters'; }
</script>
</body>
</html>
)=====";

void handleChangePassword()
{
  if (!requireAuth()) return;

  if (server.method() == HTTP_POST)
  {
    String current = server.arg("current");
    String newpass = server.arg("newpass");
    String confirm = server.arg("confirm");
    current.trim();
    newpass.trim();
    confirm.trim();

    if (simpleHash(current) != config.loginHash)
    {
      server.sendHeader("Location", "/changepass?err=wrong", true);
      server.send(302, "text/plain", "");
      return;
    }
    if (newpass != confirm)
    {
      server.sendHeader("Location", "/changepass?err=mismatch", true);
      server.send(302, "text/plain", "");
      return;
    }
    if (newpass.length() < 4)
    {
      server.sendHeader("Location", "/changepass?err=short", true);
      server.send(302, "text/plain", "");
      return;
    }

    config.loginHash = simpleHash(newpass);
    WriteConfig();
    Serial.println("Password changed");
    server.sendHeader("Location", "/changepass?ok=1", true);
    server.send(302, "text/plain", "");
    return;
  }

  server.send(200, "text/html", PAGE_ChangePassword);
}
