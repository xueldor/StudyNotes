## PKCS

* PKCS#7  PKCS#7 格式里面存储一个或多个证书。只包含证书（**certificates** ）不包含私钥。用X.509证书可以对数据进行签名、加密。一般存在.p7b 或 .p7c文件里。换句话说，一个P7B 文件，里面只有证书或证书链。
* *PKCS #8*   是一个专门用来存储私钥的格式规范
* PKCS#10 定义了证书请求格式。向CA请求颁发证书时，发出的请求就是这种标准格式。然后CA返回一个X.509证书。
* PKCS#12 包含一个或多个证书，带私钥。一般用在windows上，存放在pfx 或.p12文件里，因为带私钥，所以一般带密码保护，需要提供密码才能正确解析拿到公私钥和证书。

安卓源码里的平台签名文件platform.pk8和platform.x509.pem，顾名思义，前者是私钥，后者是证书

## PEM 文件和der文件、cer文件

以上PKCS几种格式只定义了数据格式内容，具体存文件时怎么存有两种，一种是pem文件，里面内容是ASCII或Base64编码的数据，文件后缀可以是.pem, .crt, .cer, .key 等。打开这种文件，内容一般都是这样：

```
-----BEGIN CERTIFICATE-----

BASE64编码

-----END CERTIFICATE-----
```

当然PEM不是仅仅可以编码证书，也可以private keys, public keys：

```
-----BEGIN <whatever>----- 
data
-----END <whatever>----

-----BEGIN  PRIVATE KEY-----
base 64 encoding of the private key
-----END  PRIVATE KEY-----

-----BEGIN CERTIFICATE REQUEST-----
请求证书
-----END CERTIFICATE REQUEST-----
```



DER文件是二进制格式，也是用来编码证书，证书的结构使用ASN.1（Abstract Syntax Notation One 一种数据描述语言）描述。说到底，der和pem 里面的实质性内容都是一样的，只是编码方式不一样。一个编码成base64一个编码后是二进制。

.cer扩展名

.cer指的是证书certificate，通常是DER编码格式的，但是windows也可以接受PEM格式。



CCS5.0 bootstrap证书申请流程：

1. 生成秘钥对

   ```
       // 生成KeyPair
       public static KeyPair generateKeyPair() throws NoSuchAlgorithmException, NoSuchProviderException {
           KeyPairGenerator keyPairGenerator = KeyPairGenerator.getInstance("RSA");
           // 加密规格为128/256位   ===> 1024/2048
           keyPairGenerator.initialize(2048, new SecureRandom());
           return keyPairGenerator.genKeyPair();
   
       }
   ```

   

2. 生成证书请求文件

```
public static String generatePKCS10(KeyPair keyPair) throws Exception {
        // 证书签名算法
        String sigAlg = "SHA256withRSA";
        // 各种基本信息
        String params = "CN=NH42TA2201010001,OU=A-IVI LGE,O=LGE,C=CN";

        // CN和公钥
        PKCS10CertificationRequestBuilder p10Builder = new JcaPKCS10CertificationRequestBuilder(
                new X500Name(params), keyPair.getPublic());
        // 签名算法
        JcaContentSignerBuilder csBuilder = new JcaContentSignerBuilder(sigAlg);
        csBuilder.setProvider(new BouncyCastleProvider());

        ContentSigner signer = csBuilder.build((RSAPrivateKey) keyPair.getPrivate());
        // 生成PKCS10的二进制编码格式(ber/der)
        PKCS10CertificationRequest p10 = p10Builder.build(signer);

        //将二进制格式转换为证书格式(csr)
        PemObject pemObject = new PemObject("CERTIFICATE REQUEST", p10.getEncoded());
        StringWriter str = new StringWriter();
        JcaPEMWriter jcaPEMWriter = new JcaPEMWriter(str);
        jcaPEMWriter.writeObject(pemObject);
        jcaPEMWriter.close();
        str.close();
        // base64便于网络传输
        return str.toString();
    }
    
//将str输出到一个文件里
String str = generatePKCS10(mKeyPair);
SDCardHelper.saveFileToSDCardPrivateFilesDir(str.getBytes(), "pem", "csr.pem", MainActivity.this);
Log.i(TAG, "CSR_Str: " + str);
```

