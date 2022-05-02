# MSX 似非SCC (SST Flash version)  

HRA!(@thara1129)さんが思いついた。  
似非SCC SST Flash Versionの書き込みプログラムサンプルです。  


## ■ SCCとの結線方法について
HRA!(@thara1129)さんの方法通りです。A12とMA15を入れ替えます。
<img src="./Schematic/eseSSC_SST_wiring.png" width=800>  

## ■ Flashの制御方法
BANK0=0x01/BANK1=0x06に設定することで、
Flashのコントロールアドレス2AAAhは4AAAh・コントロールアドレス5555hは6555hにマッピングされます。  


SST系のFlashは、上位アドレスを見てしまうので従来の結線通りにしてしまうと、コントロールアドレスに  
書き込み時にバンクアドレスが切り替わってしまい。上手くコントロールできません。  
それを回避するためにA12とMA15を入れ替えてその問題に対応しています。

なお、CMD体系自体は、AMD系とほぼ同じです。  
BUSYの出方はAMD系と異なるため注意してください。  

参考：  
https://www.microchip.com/wwwproducts/en/SST39SF010A  


## ■ 基板頒布について
計画中です。

## ■ Flash書き込みプログラム

●書き込み  
`>sstscc.com [書き込みFile名]`  

ソースコードは、z88dkでコンパイル可能です。コンパイルオプションは下記になります。  
`zcc +msx -create-app -subtype=msxdos -lmsxbios  main.c -o xxxx.com`  
  
