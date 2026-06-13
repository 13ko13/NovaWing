#pragma once
#include <memory>

template <typename T>
std::shared_ptr<T> WeakToShared(std::weak_ptr<T> weakPtr)
{
	return weakPtr.lock();
}