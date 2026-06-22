#pragma once
#include <memory>
#include "Game/GameObjects/GameObject.h"

/// <summary>
/// week_ptr<T>型の変数をshared_ptr<T>型に変換する
/// </summary>
/// <typeparam name="T">変換させたいオブジェクト</typeparam>
/// <param name="weakPtr">参照カウントが増えないポインタ(借りてくるだけ)</param>
/// <returns>変換後のshared_ptr<T>型の変数</returns>
template <typename T>
std::shared_ptr<T> WeakToShared(std::weak_ptr<T> weakPtr)
{
	return weakPtr.lock();
}

/// <summary>
/// shared_ptr<GameObject>型を派生クラスのshared_ptr<To>型にキャストしたものを返す
/// </summary>
/// <typeparam name="To">キャストしたいshared_ptr<T>型のポインタ</typeparam>
/// <param name="pObject">shared_ptr<GameObject>型のポインタ</param>
/// <returns></returns>
template <typename To>
std::shared_ptr<To> GameObjectToDerived(std::shared_ptr<GameObject> pObject)
{
	//キャストして返す
	return std::static_pointer_cast<To>(pObject);
}