# NerdRTOS代码规范

## 1) 缩进
缩进采用4个空格，而非tab。

## 2) 把长的行和字符串打散
每一行的长度的限制是 80 列。

## 3) 大括号和空格的放置
把起始大括号放在行尾，而把结束大括号放在行首，如：
```
if (x is true) {
    we do y
}
```
不过，有一个例外，那就是函数：函数的起始大括号放置于下一行的开头,如：
```
int function(int x)
{
    body of function
}
```
## 4) 空格
NerdRTOS的空格使用方式取决于它是用于函数还是关键字。关键字后要加一个空格。值得注意的例外是 sizeof, typeof, alignof 和 __attribute__，这 些关键字某些程度上看起来更像函数。

所以在这些关键字之后放一个空格:
```
if, switch, case, for, do, while
```
但是不要在 sizeof, typeof, alignof 或者 __attribute__ 这些关键字之后放空格。 例如:
```
s = sizeof(struct file);
```
当声明指针类型或者返回指针类型的函数时， * 的首选使用方式是使之靠近变量名 或者函数名，而不是靠近类型名。例子：
```
char *rtos_banner;
unsigned long long memparse(char *ptr, char **retptr);
char *match_strdup(substring_t *s);
```
在大多数二元和三元操作符两侧使用一个空格，例如下面所有这些操作符:
```
=  +  -  <  >  *  /  %  |  &  ^  <=  >=  ==  !=  ?  :
```
但是一元操作符后不要加空格:
```
&  *  +  -  ~  !  sizeof  typeof  alignof  __attribute__  defined
```
后缀自加和自减一元操作符前不加空格:
```
++  --
```
前缀自加和自减一元操作符后不加空格:
```
++  --
```

## 5）注释
注释是好的，不过有过度注释的危险。永远不要在注释里解释你的代码是如何运作的： 更好的做法是让别人一看你的代码就可以明白。

注释应该告诉别人你的代码做了什么，而不是怎么做的。

不要把注释放在一个函数体内部。

长 (多行) 注释的首选风格是：
```
/*
 * This is the preferred style for multi-line
 * comments in the NedrRTOS kernel source code.
 * Please use it consistently.
 *
 * Description:  A column of asterisks on the left side,
 * with beginning and ending almost-blank lines.
 */
 ```

## 6) 宏，枚举和RTL
用于定义常量的宏的名字及枚举里的标签需要大写。
```
#define CONSTANT 0x12345
```
- 在定义几个相关的常量时，最好用枚举。

- 宏的名字请用大写字母，不过形如函数的宏的名字可以用小写字母。

- 如果能写成内联函数就不要写成像函数的宏。

- 含有多个语句的宏应该被包含在一个 do-while 代码块里：
```
#define macrofun(a, b, c)           \
    do {                            \
        if (a == 5)                 \
            do_this(b, c);          \
        } while (0)

```

## 7)条件编译
只要可能，就不要在 .c 文件里面使用预处理条件 (#if, #ifdef)；这样做让代码更难,阅读并且更难去跟踪逻辑。替代方案是，在头文件中用预处理条件提供给那些 .c 文件 使用，再给 #else 提供一个空桩 (no-op stub) 版本，然后在 .c 文件内无条件地调用 那些 (定义在头文件内的) 函数。这样做，编译器会避免为桩函数 (stub) 的调用生成 任何代码，产生的结果是相同的，但逻辑将更加清晰。

最好倾向于编译整个函数，而不是函数的一部分或表达式的一部分。与其放一个 ifdef 在表达式内，不如分解出部分或全部表达式，放进一个单独的辅助函数，并应用预处理 条件到这个辅助函数内。

如果你有一个在特定配置中，可能变成未使用的函数或变量，编译器会警告它定义了但 未使用，把它标记为 __maybe_unused 而不是将它包含在一个预处理条件中。(然而，如 果一个函数或变量总是未使用，就直接删除它。)

在代码中，尽可能地使用 IS_ENABLED 宏来转化某个 Kconfig 标记为 C 的布尔 表达式，并在一般的 C 条件中使用它：
```
if (IS_ENABLED(CONFIG_SOMETHING)) {
        ...
}
```
编译器会做常量折叠，然后就像使用 #ifdef 那样去包含或排除代码块，所以这不会带 来任何运行时开销。然而，这种方法依旧允许 C 编译器查看块内的代码，并检查它的正确性 (语法，类型，符号引用，等等)。因此，如果条件不满足，代码块内的引用符号就 不存在时，你还是必须去用 #ifdef。

在任何有意义的 #if 或 #ifdef 块的末尾 (超过几行的)，在 #endif 同一行的后面写下 注解，注释这个条件表达式。例如：
```
#ifdef CONFIG_SOMETHING
...
#endif /* CONFIG_SOMETHING */
```

## 8)类型及函数前缀
提供给用户使用类型及函数，统一添加nd_前缀。内核内部使用的可以选择不添加。

