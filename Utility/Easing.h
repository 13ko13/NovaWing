#pragma once

//動きの速さの変化(イージング)の関数をまとめる
//どれも0〜1の割合を受け取り、0〜1の割合を返す
namespace Easing
{
	//EaseInOutQuad
	//加速して減速する2乗イージング
	//t:経過時間の割合(0~1) 戻り値: 進んだ道のりの割合(0~1)
	float EaseInOutQuad(float t);
}
