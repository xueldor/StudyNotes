## **用 git fetch 取代 git clone，实现断点续传**

用 `git clone` 下载大型代码仓库时，一旦网络中断，后果是哭爹喊娘，但是于事无补，叫天天不应。

因为 `git clone` 没有实现断点续传，不知道开发者脑子“进了什么水”？Linus 求骂吗？;-）

没关系，用 `git fetch` 可以实现类似效果，而且极其简单。

先用 git init 创建一个空目录：

```text
$ mkdir test-repo
$ cd test-repo
$ git init
```

再在里头用 `git fetch` 要 clone 的仓库：

```text
$ git fetch https://gitee.com/tinylab/cloud-lab.git
$ git checkout -b master FETCH_HEAD
```

`git fetch` 只能一个一个 branch fetch，fetch 完，把 `FETCH_HEAD` checkout 出来新建对应的分支即可。如果 `git fetch` 中途中断网络，可以再次 `git fetch`，`git fetch` 可以续传，不至于一断网就前功尽弃。

git fetch 断点续传效果演示：[showterm](https://link.zhihu.com/?target=http%3A//showterm.io/d935b6cc3b28fb43458cb)



(fetch并不适用于ssh方式，ssh方式每次都会从头开始)