需求：将一个WebView的显示，以png的图像格式保存下来。

```java
//1
WebView.enableSlowWholeDocumentDraw();
//2
mProtocolWebView.loadUrl(CONSTANT.USER_PROTOCOL_URL);
//3
Bitmap map =Bitmap.createBitmap(mProtocolWebView.getWidth(),mProtocolWebView.getContentHeight(),Bitmap.Config.ARGB_8888);
                WebView.enableSlowWholeDocumentDraw();
                Canvas canvas = new Canvas(map);
                mProtocolWebView.draw(canvas);
                try {
                    FileOutputStream out = new FileOutputStream(new File("/sdcard/yhxy.png"));
                    map.compress(Bitmap.CompressFormat.PNG, 100, out);
                    out.flush();
                    out.close();
                } catch (FileNotFoundException e) {
                    e.printStackTrace();
                } catch (IOException e) {
                    e.printStackTrace();
                }
```



1. mProtocolWebView.getHeight() 获取的是显示的高度，不是里面的内容的高度。当WebView里面的内容比较多的时候，界面是可以上下滑动的，这是应该用getContentHeight()方法获取bitmap的高度。
2. 在loadUrl之前，应该调用WebView.enableSlowWholeDocumentDraw();，否则只有界面显示的一块地方有图像，其它地方是透明。
3. draw(canvas)将view画到bitmap上面。
4. bitmap.compress(Bitmap.CompressFormat.PNG, 100, out); 将bitmap输出到文件，编码为png。100是最高质量。