adb root
adb remount
adb push %1 /vendor/app/DFL_SystemSettings/DFL_SystemSettings.apk
adb shell sync
pause