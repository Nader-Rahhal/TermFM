#include <array>
#include <unordered_set>

template <typename T, size_t N>
class StaticStack {
    std::unordered_set<std::string> urls;
    std::array<T, N> data;
    size_t count = 0;
    size_t head  = 0;

public:
    void push(const T& value) {
        if (urls.count(value.url)) return;

        if (count == N) {
            size_t oldest = (head + 1) % N;
            urls.erase(data[oldest].url);
        }

        head = (head + 1) % N;
        data[head] = value;
        urls.insert(value.url);
        if (count < N) ++count;
    }

    template <typename Fn>
    void forEach(Fn fn) const {
        size_t idx = head;
        for (size_t i = 0; i < count; ++i) {
            fn(data[idx]);
            idx = (idx == 0 ? N - 1 : idx - 1);
        }
    }

    size_t size()  const { return count; }
    bool   empty() const { return count == 0; }
    const T& top() const { return data[head]; }
};