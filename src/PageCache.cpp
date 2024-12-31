#include "PageCache.h"

bool CacheBlockFreqComparator::operator()(CacheBlock cb1, CacheBlock cb2) {
    return cb1.req_am < cb2.req_am;
}

bool CacheBlockTimeComparator::operator()(CacheBlock cb1, CacheBlock cb2) {
    return cb1.access_time < cb2.access_time;
}

template <typename T, typename U>
std::size_t PairHasher::operator()(const std::pair<T, U>& x) const {
    return std::hash<T>()(x.first) ^ std::hash<U>()(x.second);
}