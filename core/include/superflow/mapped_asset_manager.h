// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <vector>

namespace flow
{
template<typename K, typename V = K>
class MappedAssetManager
{
public:
  void put(const K& key, const V& value);

  V get(const K& key) const;

  bool has(const K& key) const;

  void erase(const K& key);

  void clear();

  std::vector<V> getAll() const;

private:
  mutable std::mutex mutex_;
  std::map<K, V> values_;
};

// ----- Implementation -----
template<typename K, typename V>
void MappedAssetManager<K, V>::put(const K& key, const V& value)
{
  std::lock_guard<std::mutex> lock{mutex_};

  if (values_.find(key) != values_.end())
  {
    return;
  }

  values_[key] = value;
}

template<typename K, typename V>
V MappedAssetManager<K, V>::get(const K& key) const
{
  V v;

  {
    std::lock_guard<std::mutex> lock{mutex_};

    auto it = values_.find(key);

    if (it == values_.end())
    {
      throw std::invalid_argument("Attempted accessing non-existing element");
    }

    v = it->second;
  }

  return v;
}

template<typename K, typename V>
bool MappedAssetManager<K, V>::has(const K& key) const
{
  std::lock_guard<std::mutex> lock{mutex_};

  return values_.find(key) != values_.end();
}

template<typename K, typename V>
void MappedAssetManager<K, V>::erase(const K& key)
{
  std::lock_guard<std::mutex> lock{mutex_};

  auto it = values_.find(key);

  if (it == values_.end())
  {
    return;
  }

  values_.erase(it);
}

template<typename K, typename V>
void MappedAssetManager<K, V>::clear()
{
  std::lock_guard<std::mutex> lock{mutex_};

  values_.clear();
}

template<typename K, typename V>
std::vector<V> MappedAssetManager<K, V>::getAll() const
{
  std::lock_guard<std::mutex> lock{mutex_};

  std::vector<V> values;

  for (const auto& kv : values_)
  {
    values.push_back(kv.second);
  }

  return values;
}
}
