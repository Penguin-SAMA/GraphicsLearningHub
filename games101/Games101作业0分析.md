这是Games101的第一份作业，具体也就是教各位如何配环境，用本文记录一下我个人的环境配置。

我是用`xmake`进行项目构建的，直接用的是github上的一个项目：

![image-20231228154742864](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/image-20231228154742864.png)

[点击跳转到项目主页](https://github.com/star-hengxing/GAMES101-xmake)

# Windows + vscode + clangd

直接在命令行中：
```shell
git clone https://github.com/star-hengxing/GAMES101-xmake.git
```

然后:

```shell
cd src/Assignment1
xmake -y
xmake run
```

在 windows + vscode中，我们需要在.vscode文件夹中添加一个`settings.json`文件，内容如下：

```json
{
    "clangd.arguments": [
        "--compile-commands-dir=.vscode",
    ],

    "xmake.debugConfigType": "codelldb",
}
```

# Mac/Linux + neovim

首先在终端中输入：

```shell
brew install gcc xmake eigen opencv
```

Linux 用户就用对应的软件包下载。

在我的 Mac 上，要在`xmake.lua`中添加一句：

```lua
add_includedirs("/opt/homebrew/Cellar/opencv/4.8.1_5/include/opencv4/")
```

具体路径在终端中输入下面的指令查看你电脑中的路径：

```shell
brew list opencv
```

然后在执行上面的前两步验证一下，可以成功编译就行了。

# 作业0代码

```cpp
#include <cmath>
#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Dense>
#include <iostream>

int main() {
    auto angle{45 * 3.14 / 180.0f};

    Eigen::Matrix3f model;
    model << cos(angle), -sin(angle), 0,
             sin(angle),  cos(angle), 0, 
                      0,           0, 1;

    Eigen::Matrix3f trans;
    trans << 1, 0, 1,
             0, 1, 2, 
             0, 0, 1;
             
    Eigen::Vector3f p{2.0f, 1.0f, 1.0f};
    auto result = trans * model * p;
    std::cout << result << std::endl;
}
```

结果：

![image-20231228160738493](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/image-20231228160738493.png)