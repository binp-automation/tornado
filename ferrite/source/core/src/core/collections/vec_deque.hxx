#include "vec_deque.hpp"


template <typename T>
__VecDeque<T>::__VecDeque(const __VecDeque &other) : __VecDeque(other.size()) {
    append_copy_unchecked(other);
}

template <typename T>
__VecDeque<T> &__VecDeque<T>::operator=(const __VecDeque<T> &other) {
    clear();
    append_copy(other);
    return *this;
}

template <typename T>
__VecDeque<T>::__VecDeque(__VecDeque &&other) : data_(std::move(other.data_)), front_(other.front_), back_(other.back_) {
    other.front_ = 0;
    other.back_ = 0;
}

template <typename T>
__VecDeque<T> &__VecDeque<T>::operator=(__VecDeque<T> &&other) {
    clear();

    data_ = std::move(other.data_);
    front_ = other.front_;
    back_ = other.back_;

    other.front_ = 0;
    other.back_ = 0;

    return *this;
}

template <typename T>
size_t __VecDeque<T>::capacity() const {
    if (mod() > 1) {
        return mod() - 1;
    } else {
        return 0;
    }
}

template <typename T>
size_t __VecDeque<T>::size() const {
    if (mod() == 0) {
        return 0;
    } else {
        return ((back_ + mod()) - front_) % mod();
    }
}

template <typename T>
bool __VecDeque<T>::empty() const {
    return size() == 0;
}

template <typename T>
T __VecDeque<T>::pop_back_unchecked() {
    size_t new_back = (back_ + mod() - 1) % mod();
    T &ref = data_[new_back].assume_init();
    T val(std::move(ref));
    back_ = new_back;
    ref.~T();
    return val;
}

template <typename T>
T __VecDeque<T>::pop_front_unchecked() {
    size_t new_front = (front_ + 1) % mod();
    T &ref = data_[front_].assume_init();
    T val(std::move(ref));
    front_ = new_front;
    ref.~T();
    return val;
}

template <typename T>
void __VecDeque<T>::push_back_unchecked(T &&value) {
    size_t new_back = (back_ + 1) % mod();
    data_[back_].init_in_place(std::move(value));
    back_ = new_back;
}

template <typename T>
void __VecDeque<T>::push_front_unchecked(T &&value) {
    size_t new_front = (front_ + mod() - 1) % mod();
    data_[new_front].init_in_place(std::move(value));
    front_ = new_front;
}

template <typename T>
void __VecDeque<T>::append_unchecked(__VecDeque<T> &other) {
    while (other.front_ != other.back_) {
        data_[back_].init_in_place(std::move(other.data_[other.front_].assume_init()));
        other.front_ = (other.front_ + 1) % other.mod();
        back_ = (back_ + 1) % mod();
    }
}

template <typename T>
void __VecDeque<T>::append_copy_unchecked(const __VecDeque<T> &other) {
    size_t front_view = other.front_;
    while (front_view != other.back_) {
        data_[back_].init_in_place(other.data_[front_view].assume_init());
        front_view = (front_view + 1) % other.mod();
        back_ = (back_ + 1) % mod();
    }
}

template <typename T>
void __VecDeque<T>::reserve_mod(size_t new_mod) {
    if (new_mod > std::max(size_t(1), mod())) {
        __VecDeque<T> new_self(new_mod - 1);
        new_self.append_unchecked(*this);
        *this = std::move(new_self);
    }
}

template <typename T>
void __VecDeque<T>::grow() {
    if (mod() > 1) {
        reserve_mod(2 * mod());
    } else {
        reserve_mod(2);
    }
}

template <typename T>
void __VecDeque<T>::grow_to_free(size_t count) {
    size_t new_mod = std::max(mod(), size_t(2));
    while (new_mod < size() + count + 1) {
        new_mod = 2 * new_mod;
    }
    reserve_mod(new_mod);
}


template <typename T>
void __VecDeque<T>::clear() {
    if (!std::is_trivial_v<T>) {
        // Destructors aren't called automatically because of MaybeUninit.
        // Call them manually for initialized elements.
        while (front_ != back_) {
            data_[front_].assume_init().~T();
            front_ = (front_ + 1) % mod();
        }
    }
    front_ = 0;
    back_ = 0;
}

template <typename T>
void __VecDeque<T>::reserve(size_t new_cap) {
    reserve_mod(new_cap + 1);
}

template <typename T>
void __VecDeque<T>::append(__VecDeque &other) {
    reserve(size() + other.size());
    append_unchecked(other);
}

template <typename T>
void __VecDeque<T>::append_copy(const __VecDeque &other) {
    reserve(size() + other.size());
    append_copy_unchecked(other);
}

template <typename T>
std::optional<T> __VecDeque<T>::pop_back() {
    if (!empty()) {
        return pop_back_unchecked();
    } else {
        return std::nullopt;
    }
}

template <typename T>
std::optional<T> __VecDeque<T>::pop_front() {
    if (!empty()) {
        return pop_front_unchecked();
    } else {
        return std::nullopt;
    }
}

template <typename T>
void __VecDeque<T>::push_back(T &&value) {
    if (size() == capacity()) {
        grow();
    }
    return push_back_unchecked(std::move(value));
}

template <typename T>
void __VecDeque<T>::push_front(T &&value) {
    if (size() == capacity()) {
        grow();
    }
    return push_front_unchecked(std::move(value));
}

template <typename T>
void __VecDeque<T>::push_back(const T &value) {
    return push_back(T(value));
}

template <typename T>
void __VecDeque<T>::push_front(const T &value) {
    return push_front(T(value));
}

template <typename T>
size_t __VecDeque<T>::skip_front(size_t count) {
    size_t skip = 0;
    if constexpr (std::is_trivial_v<T>) {
        if (count != 0) {
            skip = std::min(count, ((back_ + mod()) - front_) % mod());
            front_ = (front_ + skip) % mod();
        }
    } else {
        while (front_ != back_ && skip < count) {
            data_[front_].assume_init().~T();
            front_ = (front_ + 1) % mod();
            skip += 1;
        }
    }
    return skip;
}

template <typename T>
size_t __VecDeque<T>::skip_back(size_t count) {
    size_t skip = 0;
    if constexpr (std::is_trivial_v<T>) {
        if (count != 0) {
            skip = std::min(count, ((back_ + mod()) - front_) % mod());
            back_ = (back_ + mod() - skip) % mod();
        }
    } else {
        while (front_ != back_ && skip < count) {
            back_ = (back_ + mod() - 1) % mod();
            data_[back_].assume_init().~T();
            skip += 1;
        }
    }
    return skip;
}

template <typename T>
std::pair<Slice<T>, Slice<T>> __VecDeque<T>::as_slices() {
    T *data = reinterpret_cast<T *>(data_.data());
    if (front_ <= back_) {
        return std::pair{
            Slice<T>{data + front_, back_ - front_},
            Slice<T>{},
        };
    } else {
        return std::pair{
            Slice<T>{data + front_, mod() - front_},
            Slice<T>{data, back_},
        };
    }
}

template <typename T>
std::pair<Slice<const T>, Slice<const T>> __VecDeque<T>::as_slices() const {
    const T *data = reinterpret_cast<const T *>(data_.data());
    if (front_ <= back_) {
        return std::pair{
            Slice<const T>{data + front_, back_ - front_},
            Slice<const T>{},
        };
    } else {
        return std::pair{
            Slice<const T>{data + front_, mod() - front_},
            Slice<const T>{data, back_},
        };
    }
}

template <typename T>
std::pair<Slice<MaybeUninit<T>>, Slice<MaybeUninit<T>>> __VecDeque<T>::free_space_as_slices() {
    if (back_ < front_) {
        return std::pair{
            Slice<MaybeUninit<T>>{&data_[back_], front_ - back_ - 1},
            Slice<MaybeUninit<T>>{},
        };
    } else {
        return std::pair{
            Slice<MaybeUninit<T>>{&data_[back_], mod() - back_ - (front_ == 0)},
            Slice<MaybeUninit<T>>{&data_[0], front_ == 0 ? 0 : front_ - 1},
        };
    }
}

template <typename T>
void __VecDeque<T>::expand_front(size_t count) {
    assert_true(count <= capacity() - size());
    front_ = ((front_ + mod()) - count) % mod();
}

template <typename T>
void __VecDeque<T>::expand_back(size_t count) {
    assert_true(count <= capacity() - size());
    back_ = (back_ + count) % mod();
}

template <typename T>
VecDequeView<T> __VecDeque<T>::view() {
    auto [first, second] = as_slices();
    return VecDequeView<T>(first, second);
}

template <typename T>
VecDequeView<const T> __VecDeque<T>::view() const {
    auto [first, second] = as_slices();
    return VecDequeView<const T>(first, second);
}

template <typename T>
size_t __VecDequeView<T>::size() const {
    return first_.size() + second_.size();
}

template <typename T>
bool __VecDequeView<T>::empty() const {
    return first_.size() == 0;
}

template <typename T>
void __VecDequeView<T>::clear() {
    *this = __VecDequeView();
}

template <typename T>
std::optional<std::reference_wrapper<T>> __VecDequeView<T>::pop_back() {
    if (!second_.empty()) {
        return second_.pop_back();
    } else {
        return first_.pop_back();
    }
}

template <typename T>
std::optional<std::reference_wrapper<T>> __VecDequeView<T>::pop_front() {
    auto ret = first_.pop_front();
    if (first_.empty() && !second_.empty()) {
        std::swap(first_, second_);
    }
    return ret;
}

template <typename T>
size_t __VecDequeView<T>::skip_front(size_t count) {
    size_t first_skip = first_.skip_front(count);
    size_t second_skip = 0;
    if (first_.empty()) {
        std::swap(first_, second_);
        second_skip = first_.skip_front(count - first_skip);
    }
    return first_skip + second_skip;
}

template <typename T>
size_t __VecDequeView<T>::skip_back(size_t count) {
    size_t second_skip = second_.skip_back(count);
    size_t first_skip = first_.skip_back(count - second_skip);
    return second_skip + first_skip;
}

template <typename T>
std::pair<Slice<T>, Slice<T>> __VecDequeView<T>::as_slices() {
    return std::pair(first_, second_);
}

template <typename T>
std::pair<Slice<const T>, Slice<const T>> __VecDequeView<T>::as_slices() const {
    return std::pair(first_, second_);
}


template <typename T>
size_t _VecDeque<T, true>::stream_read(T *data, size_t len) {
    size_t read_len = this->view().stream_read(data, len);
    assert_eq(this->skip_front(read_len), read_len);
    return read_len;
}

template <typename T>
bool _VecDeque<T, true>::stream_read_exact(T *data, size_t len) {
    if (this->view().stream_read_exact(data, len)) {
        assert_eq(this->skip_front(len), len);
        return true;
    } else {
        return false;
    }
}

template <typename T>
size_t _VecDeque<T, true>::stream_write(const T *data, size_t len) {
    // Reserve enough space for new elements.
    this->grow_to_free(len);

    // Copy data to queue.
    auto [left, right] = this->free_space_as_slices();
    size_t left_len = std::min(left.size(), len);
    memcpy(left.data(), data, sizeof(T) * left_len);
    size_t right_len = std::min(right.size(), len - left_len);
    memcpy(right.data(), data + left_len, sizeof(T) * right_len);

    assert_eq(left_len + right_len, len);
    this->expand_back(len);
    return len;
}

template <typename T>
bool _VecDeque<T, true>::stream_write_exact(const T *data, size_t len) {
    assert_eq(this->stream_write(data, len), len);
    return true;
}

template <typename T>
size_t _VecDeque<T, true>::stream_read_from(StreamRead<T> &stream, std::optional<size_t> len_opt) {
    if (len_opt.has_value()) {
        size_t len = len_opt.value();

        // Reserve enough space for new elements.
        this->grow_to_free(len);

        // Read first slice.
        auto [left, right] = this->free_space_as_slices();
        size_t left_len = std::min(left.size(), len);
        size_t left_res_len = stream.stream_read(reinterpret_cast<T *>(left.data()), left_len);
        this->expand_back(left_res_len);
        if (left_res_len < left_len) {
            return left_res_len;
        }

        // Read second slice.
        size_t right_len = std::min(right.size(), len - left_len);
        auto right_res_len = stream.stream_read(reinterpret_cast<T *>(right.data()), right_len);
        this->expand_back(right_res_len);
        return left_len + right_res_len;
    } else {
        // Read infinitely until stream ends.
        size_t total = 0;
        for (;;) {
            size_t free = this->capacity() - this->size();
            if (free > 0) {
                size_t res_len = stream_read_from(stream, free);
                total += res_len;
                if (res_len < free) {
                    return total;
                }
            }
            this->grow();
        }
    }
}

template <typename T>
size_t _VecDeque<T, true>::stream_write_into(StreamWrite<T> &stream, std::optional<size_t> len_opt) {
    size_t res_len = this->view().stream_write_into(stream, len_opt);
    assert_eq(this->skip_front(res_len), res_len);
    return res_len;
}

template <typename T>
size_t _VecDequeView<T, true>::stream_read(T *data, size_t len) {
    auto [first, second] = this->as_slices();
    size_t first_len = first.stream_read(data, len);
    size_t second_len = second.stream_read(data + first_len, len - first_len);
    size_t read_len = first_len + second_len;
    assert_eq(this->skip_front(read_len), read_len);
    return read_len;
}

template <typename T>
bool _VecDequeView<T, true>::stream_read_exact(T *data, size_t len) {
    if (len <= this->size()) {
        assert_eq(this->stream_read(data, len), len);
        return true;
    } else {
        return false;
    }
}

template <typename T>
size_t _VecDequeView<T, true>::stream_write_into(StreamWrite<T> &stream, std::optional<size_t> len_opt) {
    size_t len = len_opt.value_or(this->size());
    auto [left, right] = this->as_slices();

    // Write first slice.
    size_t left_len = std::min(left.size(), len);
    size_t left_res_len = stream.stream_write(left.data(), left_len);
    assert_eq(this->skip_front(left_res_len), left_res_len);
    if (left_res_len < left_len) {
        return left_res_len;
    }

    // Write second slice.
    size_t right_len = std::min(right.size(), len - left_len);
    size_t right_res_len = stream.stream_write(right.data(), right_len);
    assert_eq(this->skip_front(right_res_len), right_res_len);
    return left_len + right_res_len;
}
