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

//
#pragma once
#include <array>
#include <map>
#include <optional>
#include <tuple>
#include <vector>

#include "object_impl.hxx"

namespace CPPHEADERS_NS_::refl {
/**
 * Defines some useful expression validators
 */

#define INTERNAL_CPPH_pewpew(Name, T, Expr) \
    template <typename, class = void>       \
    constexpr bool Name = false;            \
    template <typename T>                   \
    constexpr bool Name<T, std::void_t<decltype(Expr)>> = true;

INTERNAL_CPPH_pewpew(has_reserve_v, T, std::declval<T>().reserve(0));
INTERNAL_CPPH_pewpew(has_emplace_back, T, std::declval<T>().emplace_back());
INTERNAL_CPPH_pewpew(has_emplace_front, T, std::declval<T>().emplace_front());
INTERNAL_CPPH_pewpew(has_emplace, T, std::declval<T>().emplace());
INTERNAL_CPPH_pewpew(has_resize, T, std::declval<T>().resize(0));

static_assert(has_reserve_v<std::vector<int>>);
static_assert(has_emplace_back<std::list<int>>);

#undef INTERNAL_CPPH_pewpew

}  // namespace CPPHEADERS_NS_::refl

namespace CPPHEADERS_NS_::refl {

/**
 * Object descriptor
 */
template <typename ValTy_>
auto get_object_metadata() -> object_sfinae_t<detail::is_cpph_refl_object_v<ValTy_>>
{
    static object_metadata_ptr inst = ((ValTy_*)nullptr)->initialize_object_metadata();

    return &*inst;
}

template <typename ValTy_>
auto get_object_metadata() -> object_sfinae_t<
        (is_any_of_v<ValTy_, bool, nullptr_t, std::string>)
        || (std::is_enum_v<ValTy_> && not has_object_metadata_initializer_v<ValTy_>)
        || (std::is_integral_v<ValTy_>)
        || (std::is_floating_point_v<ValTy_>)  //
        >
{
    static struct manip_t : if_primitive_control
    {
        primitive_t type() const noexcept override
        {
            if constexpr (std::is_enum_v<ValTy_>) { return primitive_t::integer; }
            if constexpr (std::is_integral_v<ValTy_>) { return primitive_t::integer; }
            if constexpr (std::is_floating_point_v<ValTy_>) { return primitive_t::floating_point; }
            if constexpr (std::is_same_v<ValTy_, nullptr_t>) { return primitive_t::null; }
            if constexpr (std::is_same_v<ValTy_, bool>) { return primitive_t::boolean; }
            if constexpr (std::is_same_v<ValTy_, std::string>) { return primitive_t::string; }

            return primitive_t::invalid;
        }

        void archive(archive::if_writer* strm, const void* pvdata, object_metadata_t desc_self, optional_property_metadata opt_as_property) const override
        {
            if constexpr (std::is_enum_v<ValTy_>)
            {
                *strm << *(std::underlying_type_t<ValTy_> const*)pvdata;
            }
            else
            {
                *strm << *(ValTy_ const*)pvdata;
            }
        }

        void restore(archive::if_reader* strm, void* pvdata, object_metadata_t desc_self, optional_property_metadata opt_as_property) const override
        {
            if constexpr (std::is_enum_v<ValTy_>)
            {
                *strm >> *(std::underlying_type_t<ValTy_>*)pvdata;
            }
            else
            {
                *strm >> *(ValTy_*)pvdata;
            }
        }
    } manip;

    static auto desc = object_metadata::primitive_factory::define(sizeof(ValTy_), &manip);
    return &*desc;
}

/*
 * Fixed size arrays
 */
namespace detail {
template <typename ElemTy_>
object_metadata_t fixed_size_descriptor(size_t extent, size_t num_elems)
{
    static struct manip_t : if_primitive_control
    {
        primitive_t type() const noexcept override
        {
            return primitive_t::tuple;
        }
        const object_metadata* element_type() const noexcept override
        {
            return get_object_metadata<ElemTy_>();
        }
        void archive(archive::if_writer* strm,
                     const void* pvdata,
                     object_metadata_t desc,
                     optional_property_metadata) const override
        {
            assert(desc->extent() % sizeof(ElemTy_) == 0);
            auto n_elem = desc->extent() / sizeof(ElemTy_);
            auto begin  = (ElemTy_ const*)pvdata;
            auto end    = begin + n_elem;

            strm->array_push(n_elem);
            std::for_each(begin, end, [&](auto&& elem) { *strm << elem; });
            strm->array_pop();
        }
        void restore(archive::if_reader* strm,
                     void* pvdata,
                     object_metadata_t desc,
                     optional_property_metadata) const override
        {
            assert(desc->extent() % sizeof(ElemTy_) == 0);
            auto n_elem = desc->extent() / sizeof(ElemTy_);
            auto begin  = (ElemTy_*)pvdata;
            auto end    = begin + n_elem;

            std::for_each(begin, end, [&](auto&& elem) { *strm >> elem; });
        }
    } manip;

    static auto desc = object_metadata::primitive_factory::define(extent, &manip);
    return &*desc;
}

template <typename>
constexpr bool is_stl_array_v = false;

template <typename Ty_, size_t N_>
constexpr bool is_stl_array_v<std::array<Ty_, N_>> = true;

}  // namespace detail

template <typename ValTy_>
auto get_object_metadata() -> object_sfinae_t<std::is_array_v<ValTy_>>
{
    return detail::fixed_size_descriptor<std::remove_extent_t<ValTy_>>(
            sizeof(ValTy_),
            std::size(*(ValTy_*)0));
}

template <typename ValTy_>
auto get_object_metadata() -> object_sfinae_t<detail::is_stl_array_v<ValTy_>>
{
    return detail::fixed_size_descriptor<typename ValTy_::value_type>(
            sizeof(ValTy_),
            std::size(*(ValTy_*)0));
}

/*
 * Dynamic sized list-like containers
 *
 * (set, map, vector ...)
 */
namespace detail {

template <typename Container_>
auto get_list_like_descriptor() -> object_metadata_t
{
    using value_type = typename Container_::value_type;

    static struct manip_t : if_primitive_control
    {
        primitive_t type() const noexcept override
        {
            return primitive_t::array;
        }
        object_metadata_t element_type() const noexcept override
        {
            return get_object_metadata<value_type>();
        }
        void archive(archive::if_writer* strm,
                     const void* pvdata,
                     object_metadata_t desc,
                     optional_property_metadata) const override
        {
            auto container = reinterpret_cast<Container_ const*>(pvdata);

            strm->array_push(container->size());
            {
                for (auto& elem : *container)
                    *strm << elem;
            }
            strm->array_pop();
        }
        void restore(archive::if_reader* strm,
                     void* pvdata,
                     object_metadata_t desc,
                     optional_property_metadata) const override
        {
            auto container = reinterpret_cast<Container_*>(pvdata);
            container->clear();

            if (not strm->is_array_next())
                throw error::invalid_read_state{}.set(strm);

            // reserve if possible
            if constexpr (has_reserve_v<Container_>)
                if (auto n = strm->num_elem_next(); n != ~size_t{})
                    container->reserve(n);

            archive::context_key key;
            strm->context(&key);

            while (not strm->should_break(key))
            {
                if constexpr (has_emplace_back<Container_>)  // maybe vector, list, deque ...
                    *strm >> container->emplace_back();
                else if constexpr (has_emplace_front<Container_>)  // maybe forward_list
                    *strm >> container->emplace_front();
                else if constexpr (has_emplace<Container_>)  // maybe set
                    *strm >> *container->emplace().first;
                else
                    Container_::ERROR_INVALID_CONTAINER;
            }
        }
        requirement_status_tag status(const void* pvdata) const noexcept override
        {
            return if_primitive_control::status(pvdata);
        }
    } manip;

    constexpr auto extent = sizeof(Container_);
    static auto desc      = object_metadata::primitive_factory::define(extent, &manip);
    return &*desc;
}
}  // namespace detail

template <typename ValTy_>
auto get_object_metadata()
        -> object_sfinae_t<
                is_template_instance_of<ValTy_, std::vector>::value
                || is_template_instance_of<ValTy_, std::list>::value>
{
    return detail::get_list_like_descriptor<ValTy_>();
}

inline void compile_test()
{
    get_object_metadata<std::vector<int>>();
    get_object_metadata<std::list<std::vector<int>>>();
}

}  // namespace CPPHEADERS_NS_::refl

namespace CPPHEADERS_NS_ {
/**
 *
 * @tparam Container_
 * @tparam Adapter_
 */
template <typename Container_,
          class = std::enable_if_t<std::is_trivial_v<typename Container_::value_type>>>
class binary : public Container_
{
   public:
    using Container_::Container_;

    enum
    {
        is_container  = true,
        is_contiguous = false
    };
};

template <typename Container_>
class binary<Container_,
             std::enable_if_t<
                     std::is_trivial_v<
                             remove_cvr_t<decltype(*std::data(std::declval<Container_>()))>>>>
        : public Container_
{
   public:
    using Container_::Container_;

    enum
    {
        is_container  = true,
        is_contiguous = true
    };
};

template <typename ValTy_>
class binary<ValTy_, std::enable_if_t<std::is_trivial_v<ValTy_>>> : ValTy_
{
   public:
    using ValTy_::ValTy_;
};

static_assert(binary<std::vector<int>>::is_contiguous);
static_assert(not binary<std::list<int>>::is_contiguous);

template <typename Container_>
refl::object_metadata_ptr
initialize_object_metadata(refl::type_tag<binary<Container_>>)
{
    using binary_type = binary<Container_>;
    using value_type  = typename Container_::value_type;

    static struct manip_t : refl::if_primitive_control
    {
        refl::primitive_t type() const noexcept override
        {
            return refl::primitive_t::binary;
        }
        void archive(archive::if_writer* strm,
                     const void* pvdata,
                     refl::object_metadata_t desc,
                     refl::optional_property_metadata prop) const override
        {
            auto container = static_cast<binary_type const*>(pvdata);

            if constexpr (binary_type::is_contiguous)  // list, set, etc ...
            {
                *strm << const_buffer_view{*container};
            }
            else
            {
                auto total_size = sizeof(value_type) * std::size(*container);

                strm->binary_push(total_size);
                for (auto& elem : *container)
                {
                    strm->binary_write_some(const_buffer_view{&elem, 1});
                }
                strm->binary_pop();
            }
        }
        void restore(archive::if_reader* strm,
                     void* pvdata,
                     refl::object_metadata_t desc,
                     refl::optional_property_metadata prop) const override
        {
            // TODO ...
            auto container = static_cast<binary_type*>(pvdata);
            auto binsize   = strm->next_binary_size();
            auto elemsize  = binsize / sizeof(value_type);

            if (binsize % sizeof(value_type) != 0)
                throw refl::error::primitive{}.set(strm).message("Byte data alignment mismatch");

            if constexpr (binary_type::is_contiguous)
            {
                if constexpr (refl::has_resize<Container_>)
                {
                    // If it's dynamic, read all.
                    container->resize(elemsize);
                }

                bool const buffer_small_than_recv = std::size(*container) < elemsize;
                strm->binary_read_some({std::data(*container), std::size(*container)});

                if (buffer_small_than_recv)
                {
                    // Only take part of data.
                    strm->binary_break();
                }
            }
            else
            {
                if constexpr (refl::has_reserve_v<Container_>)
                {
                    container->reserve(elemsize);
                }

                value_type* mutable_data = {};
                for (auto idx : perfkit::count(elemsize))
                {
                    if constexpr (refl::has_emplace_back<Container_>)  // maybe vector, list, deque ...
                        mutable_data = &container->emplace_back();
                    else if constexpr (refl::has_emplace_front<Container_>)  // maybe forward_list
                        mutable_data = &container->emplace_front();
                    else if constexpr (refl::has_emplace<Container_>)  // maybe set
                        mutable_data = &*container->emplace().first;
                    else
                        Container_::ERROR_INVALID_CONTAINER;

                    strm->binary_read_some({mutable_data, 1});
                }
            }
        }
    } manip;

    return refl::object_metadata::primitive_factory::define(sizeof(Container_), &manip);
}

inline void compile_test()
{
    refl::get_object_metadata<binary<std::vector<int>>>();
}

}  // namespace CPPHEADERS_NS_

namespace CPPHEADERS_NS_::refl {

}
