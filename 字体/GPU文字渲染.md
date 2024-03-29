[快手Ytech微信号：gh_10cca253b761](https://posts.careerengine.us/author/600d64b6b6678f3380ad1dd5/posts) 发表于 2021年03月25日
GPU文字渲染

# 一、简介

  “文字特效”是短视频创作者使用得最多的功能之一。

为了制作更炫酷、更丰富、更好用的文字特效，我们自研了文字特效引擎-仓颉，并落地到快影app（快手旗下视频编辑手机应用），实现了快影app的静态花字、动态花字、文字动画等功能。

而在视频中添加文字并不像想象的那么简单，利用 GPU 加速绘制，就需要将文字转化成 GPU 能够处理的基本图元，而这其中还包括大量有趣的算法，我们在这里向大家逐一介绍。

# 二、在GPU中渲染文字

在GPU中渲染文字，一般有以下几种方法：

## 1、图片文字

图片文字即是把所用到的文字先绘制到一张图片上，记录下所有文字的坐标，当需要渲染某个文字时，只需要渲染文字在图片中对应的像素即可，很容易理解。

这个方法是一个非常普遍的GPU渲染文字的做法，除了渲染常规字体外，它还能渲染一些设计师特制的艺术字体，也能渲染emoji😁等特殊字符。

生成文字图片的方法有很多，比如提前使用工具生成（适用于英文字体或者固定文本）或者运行时使用系统API来生成（使用Canvas绘制到Bitmap上等）。

渲染图片文字的开销非常小，使用最简单的着色器即可渲染，但它的缺点也很明显，因为文字是一张图片，放大后文字会变模糊。

## 2、三角化文字 

三角形是GPU的基本图元之一，三角形可以唯一确定一个面，无论模型多么复杂，一般都可以通过一个个三角形拼装起来。既然GPU这么喜欢三角形，我们是不是也可以用三角形来拼装一个文字呢？就像这样：

2.1 获取一个字体文件，常见的字体文件如 TTF、OTF 等

2.2 从字体文件中读取文字的轮廓。字体的轮廓一般由一组或者多组直线、圆弧和贝塞尔曲线组成。可以使用第三方开源库如https://www.freetype.org/

2.3 通过轮廓构造三角形。有很多方法可以三角化一个闭合的多边形，比如耳切法 https://github.com/mapbox/earcut

2.4 使用GPU渲染这些三角形

但这样做有一些缺点：

- 必须要细分出足够多的三角形，才能高质量的还原弧线

- 必须要使用有效的抗锯齿方法来避免边缘锯齿，如SSAA（Supersampling Anti-Aliasing）、MSAA（Multisample Anti-Aliasing）等

## 3、矢量文字

有同学可能要问了：在上一个方案中，既然我们已经从字体文件中读取了文字的轮廓，而字体的轮廓是一组贝塞尔曲线或者直线组成，也就是说，文字本身就是一个矢量图形，那我们能直接渲染这个矢量图形吗？

答案是可以的。

- NVIDIA提供了一个OpenGL扩展(NV_path_rendering)，以在GPU上绘制矢量图形，这种方法简单但不具备通用性。具体可以参考https://developer.nvidia.com/gpu-accelerated-path-rendering
- 除了扩展之外，NVIDIA还介绍了一种通用的在GPU上绘制矢量图形的方法，这种方法仍然是在渲染三角形，不过特定的算法可以使得三角形能够完全包裹住曲线边缘，以方便在着色器中渲染曲线。具体可以参考https://developer.nvidia.com/gpugems/gpugems3/part-iv-image-effects/chapter-25-rendering-vector-art-gpu



我们很容易找到这样的支持GPU矢量绘图的图形库，例如：

- https://skia.org/
- https://www.cairographics.org/
- https://github.com/memononen/nanovg

上面这些库也能直接支持在GPU上渲染文字。

如果没有特殊的需求，直接绘制矢量文字也是一个比较好的解决方案了。优点是无论放大还是缩小，文字质量都有保证；缺点是矢量绘图的着色器复杂度高，GPU开销也会比较大。

## 4、有向距离场

有向距离场（Signed Distance Field，简称SDF）能同时兼备矢量文字放大后不糊的优点，又具备图片文字渲染开销非常小的优点。它是Valve于2007年提出的用于改善矢量图形渲染的方法https://cdn.cloudflare.steamstatic.com/apps/valve/2007/SIGGRAPH2007_AlphaTestedMagnification.pdf 。

有向距离场仍然使用一张图片来保存文字信息，跟图片文字不同的是，图片中存储的不是文字的颜色值，而是当前像素距离文字边缘的距离。例如下图，黑色线条是边缘，每个像素中存储的值表示当前像素距离黑色边缘的距离，“负数”表示位于图像外，“正数”表示图像内。

不过由于一般8位深度的图片中只能存储0~255的值，因此，我们一般使用"0~127(着色器中的0~0.5)"表示图像外，使用"127~255(着色器中的0.5~1)"表示图像内。使用SDF表示的文字图片看起来是这样的：越亮的地方越靠近图形中心，越暗的地方离图形中心越远。不管怎么缩放这张图片，图片的明暗区域都不会发生相对变化，因为距离经过插值之后依然是距离。

在渲染文字时，指定一个距离值（例如0.5，刚好为图像边缘），如果当前像素大于0.5，即为图像内，需要渲染，如果当前像素小于0.5，即为图像外，则不需要渲染。指定大于0.5的距离值为边缘就可以使文字变细，指定小于0.5的距离值为边缘就可以使文字变粗，指定大于一个像素的距离值作平滑即可使边缘变糊，据此可以做粗体、细体、外描边、内描边、外阴影、内阴影、外发光、内发光等各种效果。

SDF不仅可以用在文字上，它可以用在任意图形上

SDF的优势很明显，无论放大还是缩小，它都不会有任何质量损失，用来绘制字体是最好的选择之一。生成SDF有几种常用的方法：

### 4.1、EDT（Euclidean Distance Transform）算法

在常规方法中，我们需要遍历一张图片来生成对应的SDF，最暴力的方法是双重循环找最近的边缘，时间复杂度是O(n^2)。但对于二值化的二维网格，距离场中的“距离”为欧式距离，可以使用EDT（Euclidean Distance Transform）算法来加速 http://fab.cba.mit.edu/classes/S62.12/docs/Meijster_distance.pdf ，EDT算法具有O(n)复杂度，目前也有一些开源库使用EDT算法来生成SDF，例如 https://github.com/mapbox/tiny-sdf/ 。

### 4.2、EDTAA（Anti-aliased Euclidean Distance Transform）算法

EDTAA算法是在EDT算法的基础上，增加了对透明子像素的支持，当输入图尺寸较小时，还能有比较高的质量。作者不仅附上了论文，http://weber.itn.liu.se/~stegu/aadist/edtaa_preprint.pdf ，还附上了代码 http://weber.itn.liu.se/~stegu/edtaa/ ，简直是开发者福音。

EDTAA算法被应用于https://skia.org/ 、https://github.com/rougier/freetype-gl 等开源库中，运行速度很快，质量也不错。

### 4.3、根据矢量信息计算SDF

EDT算法和EDTAA算法都是扫描整张图片来生成SDF，这样做的好处是通用性较高，无需输入矢量信息就能生成，缺点是由于缺少了线段的矢量信息，质量不太稳定，可能会有折线或者锯齿等问题。

如果能直接输入矢量图形的轮廓信息，通过轮廓来生成SDF，就能避免EDT和EDTAA的缺点，因为如果知道轮廓在哪里，我们就可以使用距离公式精确的计算像素到轮廓的距离。

使用矢量图形来计算SDF，目前也有很多开源的方案，例如：

- https://github.com/Chlumsky/msdfgen
- https://github.com/behdad/glyphy

# 三、总结

本文介绍了常见的几种文字渲染方法。文字渲染是仓颉引擎的核心基础，不过，搞定文字渲染只是第一步，我们还需要搞定排版、动画、以及特效，每一个能力都富有挑战性和趣味性，通过组合这些能力，能实现丰富多彩的文字效果，使短视频创作者能更加方便快捷的给视频添加文字。未来仓颉引擎会朝着更加多样化的方向发展，我们会持续推出更多炫酷的能力以帮助短视频创作者提高作品的多样性，也给短视频消费者带来不一样的体验。



快手Y-tech介绍

  Y-tech团队是快手公司在人工智能领域的探索者和先行者，致力于计算机视觉、计算机图形学、机器学习、AR/VR等领域的技术创新和业务落地，不断探索新技术和新用户体验的最佳结合点。目前Y-tech在北京、深圳、杭州、Seattle、Pale Alto有研发团队，大部分成员来自于国际知名公司和大学。如果你对我们做的事情很感兴趣，希望一起做酷炫的东西，创造更大的价值，请联系我们：

ytechservice@kuaishou.com。