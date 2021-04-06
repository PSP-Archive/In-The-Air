Readme for ITA source code (C)

The source files could be opened in text editors or IDEs.

Bugs and comments can be forwarded to
ITA Windows version source: wzysj@citiz.net
ITA PSP version source: lylatitude@126.com

Thank dr_watson for his tutorials.

Codes of graphics and sounds are based on codes of dr_watson's tutorials.
Windows version doesn't include sound resources.
Sounds included in this source are from StarBugz(dr_watson).

Required Lib
------------------------------
libpng
mikmodlib

To build
------------------------------
Enter the directory of ITA src
make clean
make

sceneMaker.exe is used to create new scene from txt file.(DES Encryption)
...\>sceneMaker ~Seen.txt Seen.txt

To run
------------------------------
Works with firmware 1.5
Copy directory "ITAP                           _" to "ms0:\PSP\GAME\"
Cpoy directory "ITAP_~1%"                         to "ms0:\PSP\GAME\"
Memory Stick->ITA to run

Game control (The screen is rotated 90 deg)
------------------------------
Direction button or Pad: move my plane
L or CROSS:              Confirm / Fire
CIRCLE:                  Screenshot (ms0:\PSP\PHOTO\screenshot.png)
TRIANGLE:                Auto fire
SQUARE:                  Enable cheat
START(In the game):      Pop up the game menu
R:                       Debug Info -- FPS, Event Counter, Music Counter, Cheat Tag, Auto Fire Tag
HOME:                    Exit to PSP Menu

License:
--------
This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your option) any
later version.
 
This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA 02111-1307 USA

Special thanks to:
------------------
PSPSDK team
ps2dev.org
James Hui (a.k.a. Dr.Watson) -- <<StarBugz>>
Friends at pspchina.net

2006-01-15 lylatitude @ www.PSPChina.net
------------------------------------------------------------------------------------------









ITA源代码说明（C语言）

源文件可以用文本编辑器或者IDE打开。

有Bug或建议请联系
Windows版的源代码问题请联系wzysj@citiz.net
有关PSP版的源代码问题请联系lylatitude@126.com

学习时推荐看一下dr_watson的PSP游戏开发教程，会很快上手的。

图形、声音部分代码修改自dr_watson的PSP游戏开发教程。横屏显示基于正常显示的方法，不必修改原先的贴图资源。

部分声音资源来自StarBugz的资源，原Windows版游戏不包括声音资源。

PSP版与Windows版的主要区别在于增加了声音、菜单，修改了游戏状态控制。链表等操作基本一致，PSP版的绘图部分作了相应的简化。

必要的库文件
------------------------------
libpng
mikmodlib

编译链接
------------------------------
进入ITA源代码目录
make clean
make

sceneMaker.exe用来从txt文本创建新的关卡。（DES加密）
...\>sceneMaker ~Seen.txt Seen.txt

运行
------------------------------
在firmware 1.5的系统上测试通过
拷贝目录 "ITAP                           _" 到 "ms0:\PSP\GAME\"
拷贝目录 "ITAP_~1%"                         到 "ms0:\PSP\GAME\"
Memory Stick->ITA 运行

游戏操作 (屏幕旋转90度)
------------------------------
方向键或摇杆: 移动自己的飞机
L键或X键:         确定或开火
三角:             开启/关闭自动射击
方块:             开启/关闭作弊选项
CIRCLE键:         屏幕截图 (保存到ms0:\PSP\PHOTO\screenshot.png)
START键(游戏中):  弹出游戏菜单
R键:              调试信息 -- 每秒桢数, 事件计数，音乐控制计数，自动射击标志，作弊标志
HOME键:           退回到PSP系统菜单

2006-01-15 lylatitude @ www.PSPChina.net
------------------------------------------------------------------------------------------

