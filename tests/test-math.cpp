/*******************************************************************************
 * MIT License
 *
 * Copyright (c) 2021. Seungwoo Kang
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * project home: https://github.com/perfkitpp
 ******************************************************************************/

#include "math/matrix.hxx"
#include "third/doctest.h"

using namespace cpph::math;

TEST_SUITE("math.matrix")
{
    TEST_CASE("constructions")
    {
        constexpr matx33i m = matx33i::eye(), n = matx33i::create(1, 2, 3, 4, 5, 6, 7, 8, 9);
        auto c = m + (-n);

        constexpr auto cc = matx33i{}.row(2);
        static_assert(decltype(cc)::num_cols == 3);
        static_assert(decltype(cc)::num_rows == 1);

        constexpr int r = cc(0, 1);
        auto gk         = cc(4, 1);

        matx33i g = {};
        g += c;

        g* g;
    }
}
