#ifndef PSCOMMON2_NULL_OUTPUT_ITERATOR_HPP_
#define PSCOMMON2_NULL_OUTPUT_ITERATOR_HPP_

#include <memory>

namespace evo {

/** A dummy output iterator that absorbs all operations applied to it,
 essentially serving as a /dev/null for pushed output.  Useful when one wishes
 to call a function that takes an output iterator without actually receiving
 that particular output from the function.  That is, passing this to a function
 as an output iterator argument effectively disables that particular output.
 */
template <typename ValueType>
class NullOutputIterator
{
public:
  /**
   * Constructs a NullOutputIterator that contains a default-constructed
   * ValueType instance.
   */
  NullOutputIterator ()
  : punching_bag_(new ValueType())
  {}

  /** Use this constructor only if ValueType has no default constructor.
   * @param punching_bag The ValueType instance that will be absorbing
   * the unused output data.
   */
  NullOutputIterator (std::shared_ptr<ValueType> punching_bag)
  : punching_bag_(punching_bag)
  {}

  /** Copy constructs a NullOutputIterator from \p arg.
   * @param arg The NullOutputIterator that will be copied into the new instance.
   */
  NullOutputIterator (const NullOutputIterator& arg)
  : punching_bag_(arg.punching_bag_)
  {}

  ~NullOutputIterator ()
  {}

  /** Copies a NullOutputIterator's underlying ValueType instance into the
   * object.
   * @param arg The NullOutputIterator that will be copied.
   * @return A reference to the object.
   */
  NullOutputIterator& operator = (const NullOutputIterator& arg) {
    punching_bag_ = arg.punching_bag_;
    return *this;
  }

  /** Dereference operator.
   * @return A reference to the underlying ValueType instance.
   */
  ValueType&       operator * ()       { return *punching_bag_; }

  /** Const dereference operator.
   * @return A const reference to the underlying ValueType instance.
   */
  const ValueType& operator * () const { return *punching_bag_; }

  /** Pointer-to-member operator.
   * @return A pointer to the underlying ValueType instance.
   */
  ValueType*       operator -> ()       { return punching_bag_.get(); }

  /** Const pointer-to-member operator.
   * @return A const pointer to the underlying ValueType instance.
   */
  const ValueType* operator -> () const { return punching_bag_.get(); }

  /** Pre-increment operator, this is purposefully a no-op.
   * @return A reference to the object.
   */
  NullOutputIterator& operator ++ ()    { return *this; }

  /** Post-increment operator, this is purposefully a no-op.
   * @return A reference to the object.
   */
  NullOutputIterator& operator ++ (int) { return *this; }

private:

  /** The underlying ValueType instance that will absorb any writes.
   */
  std::shared_ptr<ValueType> punching_bag_;
};

/** Definition of a specialized swap() is necessary for our NullOutputIterator
 to be a fully fledged output iterator type. This is a no-op.
 */
template <typename ValueType>
void swap (NullOutputIterator<ValueType>& arg1,
           NullOutputIterator<ValueType>& arg2)
{}

} // namespace evo

#endif
