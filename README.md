# Qdreamer Toolkit

## git hooks (clang-format)

- 更改git hook路径: git config --local core.hooksPath .githooks

## 编译

- 更新第三方库 (git submodule)

    - git submodule init

    - git submodule update

- 构建工具 [xmake](https://xmake.io/#/getting_started)

- 包管理

    - xmake 官方仓库（含常用软件包） https://github.com/xmake-io/xmake-repo

    - 添加自建仓库 ``` xrepo add-repo qdr-repo http://gitea.qdreamer.lan/develop/qdr-repo ```

- 配置选项 xmake config

    - with-onnxruntime 打开onnxruntime 支持

    - with-recorder 打开录音机(alsa) 支持

    - with-asr 打开音频后端支持

    - with-bfio 打开音频前端支持

    - with-semdlg 打开语义支持

    - static-link 静态链接工具

    - with-sdk 打开SDK支持

        - sdk-use-asr 打开SDK ASR 支持

        - with-auth 打开SDK 授权需求，默认为yes

    - with-unittest 打开单元测试

- xmake内置mode( xmake f --mode=xxx )

    - debug (编译速度快，代码未优化，保留符号)

    - release (打开-O3优化，编译较慢，不保留符号)

    - releasedbg (打开-O3优化，编译较慢，保留符号)

    - profile (打开-O3优化，编译较慢，保留符号，添加-pg)

    - check (编译速度快，代码未优化，保留符号，添加-fsanitize=address，用于内存检查速度快于 valgrind)

- 交叉编译

    - 常用工具链位于共享目录 /mnt/develop/toolchain

    - 对外提供动态库时可使用xmake install -o 安装路径 打包对外输出的lib及include, lib会自动解析第三方依赖，如使用NDK25编译安卓lib

        - xmake f -c --with-onnxruntime=yes --kind=shared --ndk=/mnt/develop/toolchain/android-ndk-25c --plat=android# 依赖第三方库onnxruntime

        - xmake build qtk

        - xmake install -o /tmp/x # /tmp/x/lib下会包含libqtk.so以及依赖的libonnxruntime.so

## 常用脚本

- py/gen.py 用于生成模块框架代码，在仓库根目录下执行，如生成 qtk/cv/qtk_cv_test.{c,h,_cfg.c, _cfg.h} 使用 python py/gen.py qtk/cv/cv_test即可

## 单元测试

- 单元测试在unittest文件夹下

- unittest目录结构与源码保持一致，即wtk/core/wtk_str.c的测试代码位于unittest/wtk/core/wtk_str.cc

- TestCase命名TEST(mod, func) mod为目录+模块名以下划线隔开，func为具体测试的函数名，如

    - TEST(wtk_core_os, is_dir)为wtk/core/wtk_os.c 下wtk_is_dir的测试

    - TEST(wtk_core_json, new)为wtk/core/json/wtk_json.c下wtk_json_new的测试，最后一级文件夹跟模块重名可省略

- Bug修复后可添加对应的TestCase确保同一个Bug不重复出现

## 工作流

- 主分支保留稳定代码版本, 禁止在主分支上直接push

- 新功能开发流程

    - 切换至主分支并更新到最新版本 git checkout master; git pull

    ```sh
    git checkout master
    git pull
    ```

    - 创建以功能名称命名的新分支，在此分支上开发测试 git checkout -b new_feature

    ```sh
    git checkout -b new_feature
    ```

    - 开发过程中关注主分支的更新并且将master新提交同步到当前分支

    ```sh
    git checkout master
    git pull
    git checkout new_feature
    git rebase master
    # 测试更新后是否正常
    git push -f # 强制更新remote功能分支
    ```

    - 功能完成后发起合并请求，review代码，确认ok后合并(合并前请确保已同步master最新修改)，删除临时的功能分支

- 版本发布流程

    - 发布提测的版本需已经合并到主分支上

    - 在主分支上打tag，在 [gitea](http://gitea.qdreamer.lan/develop/qtk/tags) 上找到对应tag，然后点击发布新版，写清楚版本描述

## 开发文档

- [lex](./doc/lex.md)
