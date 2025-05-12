rm DFL_CarControl_rough.apk
rm DFL_CarControl_align.apk
rm DFL_CarControl.apk.idsig
rm DFL_CarControl.apk
apktool b ./DFL_CarControl/ -o DFL_CarControl_rough.apk
~/Android/Sdk/build-tools/34.0.0/zipalign -p -f -v 4 DFL_CarControl_rough.apk DFL_CarControl_align.apk
~/Android/Sdk/build-tools/34.0.0/apksigner sign --ks security/platform.keystore --ks-key-alias android --ks-pass pass:android --key-pass pass:android --out DFL_CarControl.apk DFL_CarControl_align.apk
if [ $? -eq 0 ]; then
    rm DFL_CarControl_rough.apk
	rm DFL_CarControl_align.apk
fi