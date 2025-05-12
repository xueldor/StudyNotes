# 放到源码security目录下，执行。 windows上，git bash支持
openssl pkcs8 -inform DER -nocrypt -in platform.pk8 -out platform.pem
openssl pkcs12 -export -in  platform.x509.pem -out platform.p12 -inkey  platform.pem -password pass:android -name platform
keytool -importkeystore -deststorepass android -destkeystore ./platform.jks -srckeystore ./platform.p12 -srcstoretype PKCS12 -srcstorepass android

echo "store password和Key password都是 android "
