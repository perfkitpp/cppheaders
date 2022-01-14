// MIT License
//
// Copyright (c) 2022. Seungwoo Kang
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// project home: https://github.com/perfkitpp

#include <iostream>
#include <sstream>
#include <variant>

#include "catch.hpp"
#include "refl/archive/debug_string_writer.hxx"
#include "refl/buffer.hxx"
#include "refl/object.hxx"

using namespace cpph;

enum class my_enum
{
    test1,
    test2,
    test3
};

namespace ns {
struct inner_arg_1
{
    std::string str1          = "str1";
    std::string str2          = "str2";
    int var                   = 133;
    bool k                    = true;
    std::array<bool, 4> bools = {false, false, true, false};
    double g                  = 3.14;

    CPPH_REFL_DEFINE_OBJECT_inline(
            str1, str2,
            var, k, bools, g);
};

struct inner_arg_2
{
    inner_arg_1 rtt    = {};
    nullptr_t nothing  = nullptr;
    nullptr_t nothing2 = nullptr;

    int ints[3] = {1, 23, 4};

    CPPH_REFL_DEFINE_TUPLE_inline(rtt, nothing, nothing2, ints);
};

struct abcd
{
    int arg0 = 1;
    int arg1 = 2;
    int arg2 = 3;
    int arg3 = 4;

    CPPH_REFL_DEFINE_OBJECT_inline(arg0, arg1, arg2, arg3);
};

struct outer
{
    inner_arg_1 arg1;
    inner_arg_2 arg2;
    std::pair<int, bool> arg                = {3, false};
    std::tuple<int, double, std::string> bb = {5, 1.14, "hello"};

    perfkit::binary<abcd> r;

    CPPH_REFL_DEFINE_TUPLE_inline(arg1, arg2, arg, bb);
};

template <typename S, typename T>
class Values
{
    S a, b, c;
};

template <typename S, typename T>
cpph::refl::object_metadata_ptr
initialize_object_metadata(cpph::refl::type_tag<Values<S, T>>)
{
    return {};
}

static auto pp_ptr = refl::get_object_metadata<Values<int, double>>();
static_assert(perfkit::is_binary_compatible_v<abcd>);

struct some_other
{
    int a, b, c;
    float f, t, r;

    outer e;
    inner_arg_2 ff;

    CPPH_REFL_DECLARE_c;
};

struct vectors
{
    std::vector<std::vector<double>> f
            = {{1., 2., 3.}, {4., 5, 6}};

    std::vector<std::list<double>> f2
            = {{1., 2., 3.}, {4., 5, 6}};

    cpph::binary<std::vector<int>> f3{1, 2, 3, 4};
    cpph::binary<std::list<int>> f4{0x5abbccdd, 0x12213456, 0x31315142};

    my_enum my_enum_value = my_enum::test3;

    std::pair<int, bool> arg                = {3, false};
    std::tuple<int, double, std::string> bb = {5, 1.14, "hello"};

    CPPH_REFL_DEFINE_OBJECT_inline(f, f2, f3, f4, my_enum_value, arg, bb);
};

struct some_other_2
{
    int a, b, c;
    float f, t, r;

    outer e;
    inner_arg_2 ff;

    CPPH_REFL_DECLARE_c;
};

}  // namespace ns

namespace cpph::refl {

}

TEMPLATE_TEST_CASE("archive", "[.]", ns::vectors)
{
    archive::debug_string_writer writer{*std::cout.rdbuf()};

    std::cout << "\n\n------- CLASS " << typeid(TestType).name() << " -------\n\n";
    writer.serialize(TestType{});

    std::cout.flush();
}

#define property_  CPPH_PROP_TUPLE
#define property2_ CPPH_PROP_OBJECT_AUTOKEY

// cpph::refl::object_metadata_ptr
// ns::some_other::initialize_object_metadata() noexcept
//{
//     INTERNAL_CPPH_ARCHIVING_BRK_TOKENS_0("a,b,c,f,t,r")
//     auto factory = cpph::refl::define_object<std::remove_pointer_t<decltype(this)>>();
//
//     cpph::macro_utils::visit_with_key(
//             INTERNAL_CPPH_BRK_TOKENS_ACCESS_VIEW(),
//             [&](std::string_view key, auto& value) {
//                 factory.property(key, this, &value);
//             },
//             a, b, c, f, t, r);
//
//     return factory.create();
// }
CPPH_REFL_DEFINE_OBJECT_c(ns::some_other, a, b, c, f, t, r, e, ff);
CPPH_REFL_DEFINE_TUPLE_c(ns::some_other_2, a, b, c, f, t, r, e, ff);
