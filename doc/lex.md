# lex语义解析

 目前提供的语义为规则语义，目前定义为lex语法;

## 工具
* 命令:
```
xmake run -w . lex -c lex.cfg -l ./test.lex  -s "我在上海"
```                                                                                                                                            
* 文件说明:
 * test.lex
```
city=上海|苏州;
export expr1=我在(${city})的(${.地名}) => inform(城市="$1",地名="$2") <0.9>;
```
 * lex.cfg
```
lexc={
	include_path=[${pwd}];
};

lexr={
	filter=[".",","," "];
	lib={
		tree_fn=${pwd}/post.bin;
	};
};
```
* 输出:
```
INPUT=> 我在苏州的徐家浜
OUPUT=> {"action":["inform"],"input":"我 在 苏 州 的 徐 家 浜","inform":{"城市":[{"_v":"苏州","_s":3,"_e":4,"prob":0.900000}],"地名":[{"_v":"徐家浜","_s":6,"_e":8,"prob":0.900000}]}}
```
* 说明:
    * lex使用参数:
```
lex usgae:
	-c configure file
	-l lex file
	-s input string
	-i input file
Example:
	 ./tool/lex -c ./res/lex.cfg -l test.lex -s "我在上海"
```
  * -c, 指定配置文件;
  * -l, 指定正则语法配置文件;
  * -s, 可选，如果需要解析一个字符串，可用-s "字符串内容"指定输入文本;
  * -i, 可选，如果需要解析一系列文本，将文本存入文本文件，逐行解析;
 * lex文件说明:
```
city=上海|苏州;
export expr1=我在(${city})的(${.地名}) => inform(城市="$1",地名="$2") <0.9>;
```
  * "city=上海|苏州;", 定义一条正则表达式变量，表达式名为"city",如果文本内容为"上海"或者"苏州"，"city"就会匹配成功;
  * "export expr1=我在(${city})的(${.地名}) => request(城市="$1",地名="$2") <0.9>;" ,定义一条输出正则表达式; 
    * export,定义一条输出正则表达式，一个lex文件中，解析时会将所有标为export的正则表达式匹配输入文本;
    * ${city}, 引用已定义的正则表达式;
    * (${city}), 捕获项;
    * ${.地名}，引用系统内建变量，系统内建变量由"."做为第一个字符，其中"地名"的内容由系统配置文件中的系统数据库存储，解析时，会在数据库中做匹配;
    * "=> inform(城市="$1",地名="$2")", 如果匹配，指定输出的语义项格式;
        * "$1", 第一个捕获项捕获内容, 即${city}的匹配内容;
        * "$2", 第二个捕获项捕获内容，即${.地名}的匹配内容;
 * "<0.9>"，指定这个表达式的概率，在有多个表达式匹配时，解析器选用概率最大的正则表达式捕获内容作为输出;
 * 输出说明:
```
{
    "action": [
        "inform"
    ],
    "input": "我 在 苏 州 的 徐 家 浜",
    "inform": {
        "城市": [
            {
                "_v": "苏州",
                "_s": a3,
                "_e": 4,
                "prob": 0.9
            }
        ],
        "地名": [
            {
                "_v": "徐家浜",
                "_s": 6,
                "_e": 8,
                "prob": 0.9
            }
        ]
    }
}
```
  * action, 当前匹配的动作类别，当前解析的动作为"inform",
  * inform, 当前inform操作的语义项内容:
   * 城市, inform包含城市语义项;
    * _v, 语义项内容;
    * _s, 语义项捕获的文本在输入文本的起始位置;
    * _e, 语义项捕获的文本在输入文本的结束位置;
    * prob, 语义项捕获概率;
### 简单匹配
* lex文件:
```
export expr1=我在(苏州) => inform(城市="$1");
```
* 例子: 
    * 正确匹配:
    ```
    INPUT=> 我在苏州的徐家浜
    OUPUT=> {"action":["inform"],"input":"我 在 苏 州 的 徐 家 浜","inform":{"城市":[{"_v":"苏州","_s":3,"_e":4}]}
    ```
    * 错误匹配:
    ```
    INPUT=> 我在啊苏州的徐家浜
    OUPUT=> {"action":[],"input":"我 在 啊 苏 州 的 徐 家 浜"}
    ```
### 任意字符匹配(.)
```
export expr1=我在.(苏州) => inform(城市="$1");
```
* 例子:
```
INPUT=> 我在啊苏州的徐家浜
OUPUT=> {"action":["inform"],"input":"我 在 啊 苏 州 的 徐 家 浜","inform":{"城市":[{"_v":"苏州","_s":4,"_e":5}]}}
```
==== 字符集合([]) ====
* lex文件:
```
export expr2=([你a\dA-C])喜欢 => inform(a="$1");
```
* 例子:
```
INPUT=> 你喜欢
OUPUT=> {"action":["inform"],"input":"你 喜 欢","inform":{"a":[{"_v":"你","_s":1,"_e":1}]}}

INPUT=> a喜欢
OUPUT=> {"action":["inform"],"input":"a 喜 欢","inform":{"a":[{"_v":"a","_s":1,"_e":1}]}}

INPUT=> 1喜欢
OUPUT=> {"action":["inform"],"input":"1 喜 欢","inform":{"a":[{"_v":"1","_s":1,"_e":1}]}}

INPUT=> B喜欢
OUPUT=> {"action":["inform"],"input":"B 喜 欢","inform":{"a":[{"_v":"B","_s":1,"_e":1}]}}
```
* 说明:
```
\d  匹配任何十进制数；它相当于类 [0-9]。
\D  匹配任何非数字字符；它相当于类 [^0-9]。
\s  匹配任何空白字符；它相当于类  [ \t\n\r\f\v]。
\S  匹配任何非空白字符；它相当于类 [^ \t\n\r\f\v]。
\w  匹配任何字母数字字符；它相当于类 [a-zA-Z0-9_]。
\W  匹配任何非字母数字字符；它相当于类 [^a-zA-Z0-9_]。
```
### 不定重复(*):0-多次
* lex文件:
```
export expr1=(你*喜欢) => inform(a="$1");
```
* 例子:
```
INPUT=> 你喜欢
OUPUT=> {"action":["inform"],"input":"你 喜 欢","inform":{"a":[{"_v":"你喜欢","_s":1,"_e":3}]}}
INPUT=> 喜欢
OUPUT=> {"action":["inform"],"input":"喜 欢","inform":{"a":[{"_v":"喜欢","_s":1,"_e":2}]}}
INPUT=> 你你喜欢
OUPUT=> {"action":["inform"],"input":"你 你 喜 欢","inform":{"a":[{"_v":"你你喜欢","_s":1,"_e":4}]}}
INPUT=> 你你你喜欢
OUPUT=> {"action":["inform"],"input":"你 你 你 喜 欢","inform":{"a":[{"_v":"你你你喜欢","_s":1,"_e":5}]}}
```
### 不定重复(+):1-多次
* lex文件:
```
export expr1=(你+喜欢) => inform(a="$1");
```
* 例子:
```
INPUT=> 你喜欢
OUPUT=> {"action":["inform"],"input":"你 喜 欢","inform":{"a":[{"_v":"你喜欢","_s":1,"_e":3}]}}
INPUT=> 喜欢
OUPUT=> {"action":[],"input":"喜 欢"}
INPUT=> 你你喜欢
OUPUT=> {"action":["inform"],"input":"你 你 喜 欢","inform":{"a":[{"_v":"你你喜欢","_s":1,"_e":4}]}}
```
### 不定重复(?)
* lex文件:
```
export expr1=(你?喜欢) => inform(a="$1");
```
* 例子:
```
INPUT=> 你喜欢
OUPUT=> {"action":["inform"],"input":"你 喜 欢","inform":{"a":[{"_v":"你喜欢","_s":1,"_e":3}]}}
INPUT=> 喜欢
OUPUT=> {"action":["inform"],"input":"喜 欢","inform":{"a":[{"_v":"喜欢","_s":1,"_e":2}]}}
INPUT=> 你你喜欢
OUPUT=> {"action":["inform"],"input":"你 你 喜 欢","inform":{"a":[{"_v":"你喜欢","_s":2,"_e":4}]}}
INPUT=> 你你你喜欢
OUPUT=> {"action":["inform"],"input":"你 你 你 喜 欢","inform":{"a":[{"_v":"你喜欢","_s":3,"_e":5}]}}
```
### 不定重复({m,n})
* lex文件:
```
export expr1=(你{1,3}喜欢) => inform(a="$1");
```
* 例子:
```
INPUT=> 你喜欢
OUPUT=> {"action":["inform"],"input":"你 喜 欢","inform":{"a":[{"_v":"你喜欢","_s":1,"_e":3}]}}
INPUT=> 喜欢
OUPUT=> {"action":[],"input":"喜 欢"}
INPUT=> 你你喜欢
OUPUT=> {"action":["inform"],"input":"你 你 喜 欢","inform":{"a":[{"_v":"你你喜欢","_s":1,"_e":4}]}}
INPUT=> 你你你喜欢
OUPUT=> {"action":["inform"],"input":"你 你 你 喜 欢","inform":{"a":[{"_v":"你你你喜欢","_s":1,"_e":5}]}}
INPUT=> 你你你你喜欢
OUPUT=> {"action":["inform"],"input":"你 你 你 你 喜 欢","inform":{"a":[{"_v":"你你你喜欢","_s":2,"_e":6}]}}
```
### 未知字符模糊匹配(.*)
* lex文件:
```
export expr1=(你.*喜欢) => inform(a="$1");
```
* 例子:
```
INPUT=> 你喜欢
OUPUT=> {"action":["inform"],"input":"你 喜 欢","inform":{"a":[{"_v":"你喜欢","_s":1,"_e":3}]}}
INPUT=> 你知道我喜欢
OUPUT=> {"action":["inform"],"input":"你 知 道 我 喜 欢","inform":{"a":[{"_v":"你知道我喜欢","_s":1,"_e":6}]}}
```
### 匹配开始(^)
* lex文件:
```
export expr=^(我在) => inform(a="$1");
```
* 例子:
```
INPUT=> 我在
OUPUT=> {"action":["inform"],"input":"我 在","inform":{"a":[{"_v":"我在","_s":1,"_e":2}]}}
INPUT=> 啊我在
OUPUT=> {"action":[],"input":"啊 我 在"}
```
### 匹配结束($)
* lex文件:
```
export expr=(我在)$ => inform(a="$1");
```
* 例子:
```
INPUT=> 我在
OUPUT=> {"action":["inform"],"input":"我 在","inform":{"a":[{"_v":"我在","_s":1,"_e":2}]}}
INPUT=> 我在那里
OUPUT=> {"action":[],"input":"我 在 那 里"}

```
### 内置变量(${.XXXX})
* lex文件:
```
export expr=我要去(${.地名}) => request(到达地名="$1");
```
* 例子:
```
INPUT=> 我要去东方明珠
OUPUT=> {"action":["request"],"input":"我 要 去 东 方 明 珠","request":{"到达地名":[{"_v":"东方明珠","_s":4,"_e":7}]}}
```
* 说明:
    * ${.地名}，引用系统内建变量，系统内建变量由"."做为第一个字符，其中"地名"由系统配置文件tbl数据库指定;
    * 配置文件说明:
```
lexr={
	filter=[".",","," "];
	tbl=${pwd}/post.bin;
};
```
  * 其中post.bin通过"python py/txt2tree2.py ./res/post post.bin"生成，res/post为内容目录，如

  ***TODO txt2tree2.py代码遗失待添加***
```
-rw------- 1 lz123 lz123 270K  7月 27 21:10 菜名.txt
-rw------- 1 lz123 lz123 5.6M  7月 27 21:10 餐厅.txt
-rw------- 1 lz123 lz123  10M  7月 27 21:10 地名.txt
...
```
   菜名.txt形如:
```
红烧圈子
清蒸梭子蟹
蒜香烧鱼
栗子馅
栗子烧肉
炒菜苋
东北家常凉菜
西瓜冰粥
鱼豆腐
广式椰蓉月饼
...
```
### 多规则
* lex文件:
```
export expr1=我要去(${.地名}) => request(到达地名="$1");
export expr2=(我喜欢) => request(a="$1");
```
* 例子:
```
INPUT=> 我要去东方明珠
OUPUT=> {"action":["request"],"input":"我 要 去 东 方 明 珠","request":{"到达地名":[{"_v":"东方明珠","_s":4,"_e":7}]}}
INPUT=> 我喜欢克鲁斯
OUPUT=> {"action":["request"],"input":"我 喜 欢 克 鲁 斯","request":{"a":[{"_v":"我喜欢","_s":1,"_e":3}]}}
```
### 表达式变量引用
* lex文件:
```
日期=今天|今日 => ("today");
export expr=(${日期})*我要去(苏州) => request(日期="$1",地点="$2");
```
* 例子:
```
INPUT=> 我要去苏州
OUPUT=> {"action":["request"],"input":"我 要 去 苏 州","request":{"地点":[{"_v":"苏州","_s":4,"_e":5}]}}
INPUT=> 今天我要去苏州
OUPUT=> {"action":["request"],"input":"今 天 我 要 去 苏 州","request":{"日期":[{"attr":{"today":{```],"地点":[{"_v":"苏州","_s":6,"_e":7}]}}
```
### 关键字
* 注释
```
#这是单行注释
/*
  这是多行注释
*/
```
* include
```
#include "${pwd}/../misc/num.lex"
#include ${pwd}/../misc/num.lex
#include ${pwd}/../misc/num.lex;
```
* import
```
#import "${pwd}/16.lex" as date
```
* end
```
#end #结束读取以下的内容
```
* exit
```
#exit #结束整个程序
```
* sort_by_prob 
```
# sort_by_prob #初始为0，将读取到的lex项按照优先度排序
```
* use_nbest
```
#use_nbest=0 #选择了一条语句之后不会再次进行匹配，默认是1
```
* 注释
 * #, 单行注释;
 * //, 单行注释;
 * /* ... */, 多行注释;
```
#这是单行注释;
//这是单行注释;
/*
   这是多行注释
*/
```
### 正则内容属性
* 内容属性/lower,upper,chn2num,min=?,max=?,skip="???",pre="aa,bb",not_pre="xxx,yyy",suc="aa,bb",not_suc="yy,dd"/
* lower:
 * lex文件:
```
export expr=(英文a/lower/) => request(a="$1");
```
 * 例子:
```
INPUT=> 英文A
OUPUT=> {"action":["request"],"input":"英 文 A","request":{"a":[{"_v":"英文a","_s":1,"_e":3}]}}
```
* upper:
 * lex文件:
```
export expr=(英文A/upper/) => request(a="$1");
```
 * 例子:
```
INPUT=> 英文a
OUPUT=> {"action":["request"],"input":"英 文 a","request":{"a":[{"_v":"英文A","_s":1,"_e":3}]}}
```
* chn2num:
 * lex文件:
```
export expr=(1/chn2num/) => request(a="$1");
```
 * 例子:
```
INPUT=> 一
OUPUT=> {"action":["request"],"input":"一","request":{"a":[{"_v":"1","_s":1,"_e":1}]}}
```
* skip:
 * lex文件:
```
export expr=(上海浦东机场)/skip="国际"/ => request(a="$1");
```
 * 例子:
```
INPUT=> 上海浦东国际机场
OUPUT=> {"action":["request"],"input":"上 海 浦 东 国 际 机 场","request":{"a":[{"_v":"上海浦东机场","_s":1,"_e":8}]}}
```
* min,max:
 * lex文件:
```
export expr=(你/min=2,max=4/喜欢) => request(a="$1");
```
 * 例子:
```
INPUT=> 你喜欢
OUPUT=> {"action":[],"input":"你 喜 欢"}
INPUT=> 你你喜欢
OUPUT=> {"action":["request"],"input":"你 你 喜 欢","request":{"a":[{"_v":"你你喜欢","_s":1,"_e":4}]}}
INPUT=> 你你你你你喜欢
OUPUT=> {"action":["request"],"input":"你 你 你 你 你 喜 欢","request":{"a":[{"_v":"你你你你喜欢","_s":2,"_e":7}]}}
```
* pre,当前内容前缀必须为所指定的内容:
 * lex文件:
```
export expr1=(包/pre="皮,烧"/) => request(a="$1")
```
 * 例子:
```
INPUT=> 皮包
OUPUT=> {"action":["request"],"input":"皮 包","request":{"a":[{"_v":"包","_s":2,"_e":2}]}}
INPUT=> 烧包
OUPUT=> {"action":["request"],"input":"烧 包","request":{"a":[{"_v":"包","_s":2,"_e":2}]}}
INPUT=> 你包
OUPUT=> {"action":[],"input":"你 包"}
```
* not_pre,当前内容前缀不能为所指定的内容: 
 * lex文件:
```
export expr1=(包/not_pre="皮,烧"/) => request(a="$1")
```
 * 例子:
```
INPUT=> 皮包
OUPUT=> {"action":[],"input":"皮 包"}
INPUT=> 烧包
OUPUT=> {"action":[],"input":"烧 包"}
INPUT=> 你包
OUPUT=> {"action":["request"],"input":"你 包","request":{"a":[{"_v":"包","_s":2,"_e":2}]}}
```
* suc,当前内容后缀须为所指定的内容: 
 * lex文件:
```
包=包/suc="你好"/;
export expr1=(${包}) => request(a="$1")
```
 * 例子:
```
INPUT=> 包大肠
OUPUT=> {"action":[],"input":"包 大 肠"}
INPUT=> 包你好
OUPUT=> {"action":["request"],"input":"包 你 好","request":{"a":[{"_v":"包","_s":1,"_e":1}]}}
```
* not_suc,当前内容后缀不能为所指定的内容: 
 * lex文件:
```
包=包/not_suc="你好"/;
export expr1=(${包}) => request(a="$1")
```
 * 例子:
```
INPUT=> 包大肠
OUPUT=> {"action":["request"],"input":"包 大 肠","request":{"a":[{"_v":"包","_s":1,"_e":1}]}}
INPUT=> 包你好
OUPUT=> {"action":[],"input":"包 你 好"}
```

### 正则句法属性
* 内容属性/lower,upper,match_more_wrd,match_more_var/
* lower:
 * lex文件:
```
export expr=(英文abc) => request(a="$1") /lower/;
```
 * 例子:
```
INPUT=> 英文ABC
OUPUT=> {"action":["request"],"input":"英 文 A B C","request":{"a":[{"_v":"英文abc","_s":1,"_e":5}]}}
```
* upper:
 * lex文件:
```
export expr=(英文ABC) => request(a="$1") /upper/;
```
 * 例子:
```
INPUT=> 英文abc
OUPUT=> {"action":["request"],"input":"英 文 a b c","request":{"a":[{"_v":"英文ABC","_s":1,"_e":5}]}}
```
* match_more_wrd, 默认为1，指定是否采用匹配单词最多的作为输出:
 * lex文件:
```
export expr=我在(上海东方明珠|上海) => request(a="$1") /match_more_wrd=0/;
export expr=我在(上海东方明珠|上海) => test(a="$1") /match_more_wrd=1/;
```
 * 例子:
```
INPUT=> 我在上海东方明珠
OUPUT=> {"action":["request","test"],"input":"我 在 上 海 东 方 明 珠","request":{"a":[{"_v":"上海","_s":3,"_e":4}]},"test":{"a":[{"_v":"上海东方明珠","_s":3,"_e":8}]}}
```
* match_more_var, 默认为1，指定是否采用捕获项最多的作为输出:
 * lex文件:
```
export expr=我在(上海)*(上海东方明珠|东方明珠) => request(a="$1",b="$2") /match_more_var=1/;
export expr=我在(上海)*(上海东方明珠|东方明珠) => test(a="$1",b="$2") /match_more_var=0/;
```
 * 例子:
```
INPUT=> 我在上海东方明珠
OUPUT=> {"action":["request","test"],"input":"我 在 上 海 东 方 明 珠","request":{"a":[{"_v":"上海","_s":3,"_e":4}],"b":[{"_v":"东方明珠","_s":5,"_e":8}]},"test":{"b":[{"_v":"上海东方明珠","_s":3,"_e":8}]}
```
### if else
* lex:
```
city=(苏州|上海|北京);

export expr1=(去|到)(${city}) => request(到达城市="$2");

/*
	如果当前的解码内容中有request语义操作，同时其对应的语义项中包含到达城市这一项，执行then,否则执行else
*/
if request.到达城市 then
	export expr2=([一|二|三])星级 => request(star="$1");
else
	export expr3=(你好) => request(echo="$1");
end
```
* 例子:
```
INPUT=> 我要去上海，给我订一家三星级酒店
OUPUT=> {"action":["request"],"input":"我 要 去 上 海 ， 给 我 订 一 家 三 星 级 酒 店","request":{"到达城市":[{"_v":"上海","_s":4,"_e":5}],"star":[{"_v":"三","_s":12,"_e":12}]}}
INPUT=> 你好
OUPUT=> {"action":["request"],"input":"你 好","request":{"echo":[{"_v":"你好","_s":1,"_e":2}]}}
```
### 修改属性命令
* lex文件:
```
city=(苏州|上海|北京);

export expr1=(去|到)(${city}) => request(到达城市="$2");

if request.到达城市 then
	add request.出发城市 苏州; //添加语义项
	#del request.到达城市; //删除语义项
	#mv request.出发城市 request.到达城市
	#cpy request.到达城市 request.出发城市 //复制语义项
end
```
* 输出:
```
INPUT=> 我要去上海，给我订一家三星级酒店
OUPUT=> {"action":["request"],"input":"我 要 去 上 海 ， 给 我 订 一 家 三 星 级 酒 店","request":{"到达城市":[{"_v":"上海","_s":4,"_e":5}],"出发城市":[{"_v":"苏州"}]}}
```
* 命令说明:
  * add，添加语义项，如"add request.出发城市 苏州;";
  * del, 删除语义项，如"del request.到达城市";
  * mv, 移动语义项，如"mv request.出发城市 request.到达城市";
  * cpy, 复制语义项，如"cpy request.到达城市 request.出发城市";