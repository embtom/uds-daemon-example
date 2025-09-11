#ifndef LIST_H_
#define LIST_H_

#include <typetraits.h>
#include <algorithm>
#include <list>
#include <memory>

namespace utils
{

template <typename ElementType, typename alloc = std::allocator<ElementType>>
class CList : public std::list<ElementType, alloc>
{
   static_assert(
      std::is_object_v<ElementType>,
      "CList ElementType have to be an object no reference or pointer");
   static_assert(
      detail::is_complete_v<ElementType>,
      "CList No forward declared types are allowed");

public:
   using allocator_type = alloc;
   using value_type = typename std::list<ElementType, alloc>::value_type;
   using pointer = typename std::list<ElementType, alloc>::pointer;
   using const_pointer = typename std::list<ElementType, alloc>::const_pointer;
   using iterator = typename std::list<ElementType, alloc>::iterator;
   using const_iterator =
      typename std::list<ElementType, alloc>::const_iterator;
   using reverse_iterator =
      typename std::list<ElementType, alloc>::reverse_iterator;
   using const_reverse_iterator =
      typename std::list<ElementType, alloc>::const_reverse_iterator;

   explicit CList(const alloc& rAlloc = allocator_type())
      : std::list<ElementType, alloc>(rAlloc)
   {
   }

   [[nodiscard]] const_pointer cdata(const_iterator it) const noexcept
   {
      if (it != this->cend())
      {
         return &(*it);
      }
      return nullptr;
   }

   [[nodiscard]] pointer data(iterator it) const noexcept
   {
      if (it != this->end())
      {
         return &(*it);
      }
      return nullptr;
   }

   [[nodiscard]] const_iterator cdata_to_iterator(
      const_pointer elm) const noexcept
   {
      return std::find_if(
         this->cbegin(), this->cend(),
         [&elm](const auto& o) { return (&o == elm); });
   }

   [[nodiscard]] iterator data_to_iterator(pointer elm) noexcept
   {
      return std::find_if(
         this->begin(), this->end(), [&elm](auto& o) { return (&o == elm); });
   }
};


// User-defined deduction guide
// https://en.cppreference.com/w/cpp/language/class_template_argument_deduction
template <typename alloc, typename ElementType = typename alloc::value_type>
CList(const alloc&) -> CList<ElementType, alloc>;

} // namespace utils

#endif /* LIST_H_ */
