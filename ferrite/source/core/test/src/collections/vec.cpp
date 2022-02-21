#include <core/collections/vec.hpp>

#include <gtest/gtest.h>

TEST(Vec, from_std_vector) {
    std::vector<int32_t> sv{0, 1, 2, 3};
    ASSERT_EQ(sv.size(), 4u);

    std::vector<int32_t> vec(std::move(sv));
    ASSERT_EQ(vec.size(), 4u);
    for (size_t i = 0; i < vec.size(); ++i) {
        ASSERT_EQ(vec[i], int32_t(i));
    }
}

TEST(Vec, into_std_vector) {
    Vec<int32_t> vec{0, 1, 2, 3};
    ASSERT_EQ(vec.size(), 4u);

    std::vector<int32_t> sv(std::move(vec));
    ASSERT_EQ(sv.size(), 4u);
    for (size_t i = 0; i < sv.size(); ++i) {
        ASSERT_EQ(sv[i], int32_t(i));
    }
}
