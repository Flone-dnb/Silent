# Silent
<p align="center">
  <img width="590" height="400" src="screenshot.png">
</p>
<br>
Silent is an extremely lightweight, high quality - low latency voice chat made for gaming.<br>
Must be used with the Silent Server application. <br>
<br>
Made with Qt Framework, Winsock2 and Windows Multimedia System.

# Latency
In accordance with the Silent version 2.23.0 the current latency is 60 ms (I think):<br>
<br>
Silent is sending audio packets which are 30 ms long each while the user holds his push-to-talk button. First, Silent will record one audio packet (30 ms) and then send it to the server (and will record more audio if the user is still holding his push-to-talk button). The user who received audio packets from the server will play them after he received the second audio packet. So, considering that the ping of all users is 0 we need 60 ms to record and send 2 audio packets, after that the user who received those packets will play them while still accepting the next audio packets.<br>
<br>
You can calculate your latency by this formula:<br>
Latency = 60 ms + (your ping + ping of the user-receiver) / 2.

# Add your style theme
The Silent has different style themes that you can change.<br>
<br>
To add your own theme you must go to the folder in which the Silent is installed. Then find folder "themes". In this folder, you will see themes that you can select in the Settings window. Copy and paste any .css file, rename it (only ASCII characters allowed) and change its content. After that, if you open the Silent - Settings Window you will see that your new theme can be selected there.<br>
<br>
If you want to see your theme in the official Silent build then you can make a pull request.

# License
Please note that starting from version 2.23.0 the Silent is releasing under the Zlib license.<br>
<br>
All versions before 2.23.0 were released under the MIT license.

# Build
Silent is built with the MSVC 2017 64 bit compiler and Qt Framework (through Qt Creator).<br>
<br>
After you've built the app don't forget to copy-paste the "sounds" and "themes" folders to the folder with the .exe file.